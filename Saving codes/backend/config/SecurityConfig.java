package com.example.backend.config;

import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.security.config.annotation.method.configuration.EnableMethodSecurity;
import org.springframework.security.config.annotation.web.builders.HttpSecurity;
import org.springframework.security.config.annotation.web.configuration.EnableWebSecurity;
import org.springframework.security.config.http.SessionCreationPolicy;
import org.springframework.security.core.authority.SimpleGrantedAuthority;
import org.springframework.security.oauth2.core.DelegatingOAuth2TokenValidator;
import org.springframework.security.oauth2.core.OAuth2Error;
import org.springframework.security.oauth2.core.OAuth2TokenValidator;
import org.springframework.security.oauth2.core.OAuth2TokenValidatorResult;
import org.springframework.security.oauth2.jwt.*;
import org.springframework.security.oauth2.server.resource.authentication.JwtAuthenticationConverter;
import org.springframework.security.web.SecurityFilterChain;

@Configuration
@EnableWebSecurity
@EnableMethodSecurity
public class SecurityConfig {

    // Le a URL do Keycloak do application.yaml
    @Value("${spring.security.oauth2.resourceserver.jwt.issuer-uri}")
    private String issuerUri;

    @Bean
    public SecurityFilterChain filterChain(HttpSecurity http) throws Exception {
        http
            .csrf(csrf -> csrf.disable()) // Desnecessário para API Rest Stateless
            
            .sessionManagement(session -> session
                .sessionCreationPolicy(SessionCreationPolicy.STATELESS) // Nao guarda estado
            )
            
            .authorizeHttpRequests(authorize -> authorize
                .anyRequest().authenticated() // Tudo precisa de token
            )
            
            .oauth2ResourceServer(oauth2 -> oauth2
                .jwt(jwt -> jwt
                    .decoder(jwtDecoder()) 
                    .jwtAuthenticationConverter(conversorDeRolesDoKeycloak()) 
                ) 
            );

        return http.build();
    }

    @Bean
    public JwtDecoder jwtDecoder() {
        // Cria o decodificador base (conectando no Keycloak para pegar a chave pública)
        NimbusJwtDecoder jwtDecoder = JwtDecoders.fromIssuerLocation(issuerUri);

        // Mantém as validacões padrao de segurança do Spring (Data de expiracao e Emissor)
        OAuth2TokenValidator<Jwt> validadoresPadrao = JwtValidators.createDefaultWithIssuer(issuerUri);

        // Cria a validacão adicional ( O token tem que ser um Access Token (typ = Bearer))
        OAuth2TokenValidator<Jwt> validadorDeToken = jwt -> {
            String tipoDoToken = jwt.getClaimAsString("typ");
            
            if ("Bearer".equals(tipoDoToken)) {
                return OAuth2TokenValidatorResult.success();
            }
            
            // Rejeita qualquer outro tipo de token mesmo que a matematica de decodificacao bata
            OAuth2Error erro = new OAuth2Error(
                    "invalid_token", 
                    "Acesso Negado: O token fornecido não é um Access Token válido. Tipo recebido: " + tipoDoToken, 
                    null
            );
            return OAuth2TokenValidatorResult.failure(erro);
        };

        // Combina as validacoes (padrao e de token)
        OAuth2TokenValidator<Jwt> validadoresCombinados = new DelegatingOAuth2TokenValidator<>(
                validadoresPadrao, 
                validadorDeToken
        );

        // Aplica no fluxo de decodificacao
        jwtDecoder.setJwtValidator(validadoresCombinados);

        return jwtDecoder;
    }


    @Bean
    public JwtAuthenticationConverter conversorDeRolesDoKeycloak() {
        JwtAuthenticationConverter converter = new JwtAuthenticationConverter();
        
        converter.setJwtGrantedAuthoritiesConverter(jwt -> {
            // Pega o objeto "realm_access" de dentro do JWT
            Map<String, Object> realmAccess = jwt.getClaim("realm_access");
            
            if (realmAccess == null || !realmAccess.containsKey("roles")) {
                return List.of(); // Se não tem roles, retorna vazio
            }
            
            // Extrai a lista de roles e coloca o prefixo "ROLE_" que o Spring exige
            List<String> rolesDoKeycloak = (List<String>) realmAccess.get("roles");
            
            return rolesDoKeycloak.stream()
                    .map(roleName -> new SimpleGrantedAuthority("ROLE_" + roleName.toUpperCase()))
                    .collect(Collectors.toList());
        });
        
        return converter;
    }
}