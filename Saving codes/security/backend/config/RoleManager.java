package com.example.backend.config;

import jakarta.annotation.PostConstruct;
import jakarta.servlet.http.HttpServletRequest;
import tools.jackson.core.type.TypeReference;
import tools.jackson.databind.ObjectMapper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import org.springframework.core.io.ClassPathResource;
import org.springframework.security.authorization.AuthorizationDecision;
import org.springframework.security.authorization.AuthorizationManager;
import org.springframework.security.core.Authentication;
import org.springframework.security.core.GrantedAuthority;
import org.springframework.security.web.access.intercept.RequestAuthorizationContext;
import org.springframework.stereotype.Component;

import java.io.IOException;
import java.io.InputStream;
import java.util.List;
import java.util.Map;
import java.util.function.Supplier;

@Component
public class RoleManager implements AuthorizationManager<RequestAuthorizationContext> {

    private static final Logger logger = LoggerFactory.getLogger(RoleManager.class);

    // A memória RAM onde o JSON vai ficar guardado
    private Map<String, Map<String, List<String>>> regrasDoJson;
    
    private final ObjectMapper objectMapper;

    // Injeção de dependência: O Spring entrega o ObjectMapper pronto aqui
    public RoleManager(ObjectMapper objectMapper) {
        this.objectMapper = objectMapper;
    }

    // Roda UMA ÚNICA VEZ quando o Spring Boot liga
    @PostConstruct
    public void carregarRegras() {
        try {
            InputStream inputStream = new ClassPathResource("regras-acesso.json").getInputStream();
            regrasDoJson = objectMapper.readValue(
                inputStream, 
                new TypeReference<Map<String, Map<String, List<String>>>>() {}
            );
            logger.info("Regras de acesso dinâmicas carregadas com sucesso!");
        } catch (IOException e) {
            throw new RuntimeException("Falha crítica: Não foi possível carregar regras-acesso.json", e);
        }
    }

    @Override
    public AuthorizationDecision authorize(Supplier<? extends Authentication> authenticationSupplier, RequestAuthorizationContext context) {
        
        HttpServletRequest request = context.getRequest();
        String urlRequested = request.getRequestURI();
        String httpMethod = request.getMethod(); 

        // Busca as regras no arquivo .json
        Map<String, List<String>> allowedMethods = regrasDoJson.get(urlRequested);
        
        // Se a rota não existe no JSON, ou o verbo não tá mapeado -> Bloqueio imediato
        if (allowedMethods == null || !allowedMethods.containsKey(httpMethod)) {
            logger.info("Bloqueado: Rota ou Verbo não mapeados no JSON (" + httpMethod + " " + urlRequested + ")");
            return new AuthorizationDecision(false); 
        }

        List<String> rolesExigidas = allowedMethods.get(httpMethod);
        logger.info("Roles necessaria para acessar: {}", rolesExigidas);

        // Pega o usuário montado pelo Spring (depois da validação do JWT)
        Authentication usuario = authenticationSupplier.get();

        logger.info("Autoridade do usuário: {}", usuario.getAuthorities());

        if (usuario == null || !usuario.isAuthenticated()) {
            return new AuthorizationDecision(false);
        }


        // O usuário tem o crachá necessário para passar?
        for (GrantedAuthority autoridadeDoUsuario : usuario.getAuthorities()) {
            if (rolesExigidas.contains(autoridadeDoUsuario.getAuthority())) {
                logger.info("Autoridade do usuário bateu com a necessaria: {}", autoridadeDoUsuario.getAuthority());
                return new AuthorizationDecision(true); // Liberado!
            }
        }

        // Logado, mas sem permissão (Vai gerar o erro 403)
        logger.info("Bloqueado: Usuário tentou acessar sem a Role correta.");
        return new AuthorizationDecision(false); 
    }
}