package com.example.demo.controller;

import java.util.List;
import java.util.Map;

import org.springframework.security.core.annotation.AuthenticationPrincipal;
import org.springframework.security.oauth2.client.OAuth2AuthorizedClient;
import org.springframework.security.oauth2.client.annotation.RegisteredOAuth2AuthorizedClient;
import org.springframework.security.oauth2.client.authentication.OAuth2AuthenticationToken;
import org.springframework.security.oauth2.core.oidc.user.OidcUser;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.client.HttpClientErrorException;
import org.springframework.web.client.RestClient;

import com.example.demo.service.UserService;

import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.http.ResponseEntity;


@RestController
public class BffController {

    private static final Logger logger = LoggerFactory.getLogger(BffController.class);

    private final RestClient restClient;
    private final UserService userService;

    public BffController(RestClient restClient, UserService userService) {
        this.restClient = restClient;
        this.userService = userService;
    }

    @GetMapping("/home")
    public String home(@AuthenticationPrincipal OidcUser principal) {
        return "Olá, voce logou com sucesso " + principal.getFullName() + 
               ". <br> Seu token JWT expira em: " + principal.getExpiresAt();
    }

    @GetMapping("/comparar-tokens")
    public Map<String, String> comparar(@AuthenticationPrincipal OidcUser principal, 
                                    @RegisteredOAuth2AuthorizedClient("keycloak") OAuth2AuthorizedClient client) {
        return Map.of(
            "id_token_que_voce_usou", principal.getIdToken().getTokenValue(),
            "access_token_correto", client.getAccessToken().getTokenValue(),
            "refresh_token", client.getRefreshToken().getTokenValue()
        );
    }

    @GetMapping("/me")
    public ResponseEntity<Map<String, Object>> getMe(
            @AuthenticationPrincipal OidcUser principal,
            OAuth2AuthenticationToken authToken,
            HttpServletRequest request,
            HttpServletResponse response) { 


        if (principal == null || authToken == null) {
            return ResponseEntity.ok(Map.of("authenticated", false));
        } 

        Map<String, Object> userInfo = userService.getAuthenticatedUserInfo(authToken, principal, request, response);

        // 3. Se o service retornou nulo, a gaveta sumiu. Força ciclo SSO!
        if (userInfo == null) {
            logger.warn("Tokens perdidos permanentemente no Redis");
            return ResponseEntity.ok(Map.of("authenticated", false));
        }
        
        return ResponseEntity.ok(userInfo);
    }

    @GetMapping("/check-role")
    public ResponseEntity<Map<String, Boolean>> checkRole(
            @RequestParam List<String> roles,
            OAuth2AuthenticationToken authToken,
            HttpServletRequest request,
            HttpServletResponse response) {
        logger.info("USUARIO ACESSOU A ROTA CHECK-ROLE");
        if (authToken == null) {
            return ResponseEntity.ok(Map.of("hasRole", false));
        }

        boolean hasRole = userService.checkUserHasRole(roles, authToken, request, response);
        logger.info("RETORNO -> HasRole = " + hasRole);
        return ResponseEntity.ok(Map.of("hasRole", hasRole));
    }

    @GetMapping("/backend")
    public String testeBackend() {

        return restClient.get()
                .uri("http://localhost:8082/teste")
                .retrieve()
                .body(String.class);
    }

    @GetMapping("/")
    public String firstPage() {
        return "<h1>Bem vindo! 👋</h1>" +
               "<br><a href='/home'>Fazer Login</a>";
    }


    @GetMapping("/buscar-tarefas")
    public String buscarTarefasUser() {
      
        return restClient.get()
                .uri("http://localhost:8082/minhas-tarefas")
                .retrieve()
                .body(String.class);
    }

   @GetMapping("/buscar-tarefas-admin")
    public String testarAdmin() {
        try {
            return restClient.get()
                    .uri("http://localhost:8082/admin/todas-tarefas")
                    .retrieve()
                    .body(String.class);
                    
        } catch (HttpClientErrorException.Forbidden e) {
            return "ACESSO NEGADO (CODE:" + e.getStatusCode() +") : Você não tem permissão de Administrador para ver isso.";
        }
    }
}
