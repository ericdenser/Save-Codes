package com.example.bff.controller;

import java.util.List;
import java.util.Map;

import org.springframework.security.core.annotation.AuthenticationPrincipal;
import org.springframework.security.oauth2.client.OAuth2AuthorizedClient;
import org.springframework.security.oauth2.client.OAuth2AuthorizedClientService;
import org.springframework.security.oauth2.client.annotation.RegisteredOAuth2AuthorizedClient;
import org.springframework.security.oauth2.client.authentication.OAuth2AuthenticationToken;
import org.springframework.security.oauth2.core.oidc.user.OidcUser;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.client.HttpClientErrorException;
import org.springframework.web.client.RestClient;

import com.example.bff.service.UserService;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.http.ResponseEntity;


@RestController
public class BffController {

    private static final Logger logger = LoggerFactory.getLogger(BffController.class);

    private final RestClient restClient;
    private final UserService userService;
    private final OAuth2AuthorizedClientService clientService;

    public BffController(RestClient restClient, UserService userService, OAuth2AuthorizedClientService clientService) {
        this.restClient = restClient;
        this.userService = userService;
        this.clientService = clientService;
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
            "access_token_correto", client.getAccessToken().getTokenValue()
        );
    }

    @GetMapping("/me")
    public ResponseEntity<Map<String, Object>> getMe(@AuthenticationPrincipal OidcUser principal, OAuth2AuthenticationToken authToken)  {
        if (principal == null || authToken == null) {
            return ResponseEntity.ok(Map.of("authenticated", false));
        } 

        OAuth2AuthorizedClient client = clientService.loadAuthorizedClient(
                authToken.getAuthorizedClientRegistrationId(),
                authToken.getName()
        );

        List<String> rolesDoUsuario = userService.getUserRoles(client);


        logger.info("/Me triggered, info: Name: {} Username: {} Roles: {}", principal.getFullName(), principal.getPreferredUsername(), rolesDoUsuario);
        
        // Tem gente logada, devolvemos os dados básicos dele.
        return ResponseEntity.ok(Map.of(
            "authenticated", true,
            "nome", principal.getFullName(),
            "username", principal.getPreferredUsername(),
            "roles", rolesDoUsuario
        ));
    }

    @GetMapping("/backend")
    public String testeBackend(
            // Essa anotação faz o Spring ir no Redis e pegar os tokens do usuário atual
            @RegisteredOAuth2AuthorizedClient("keycloak") OAuth2AuthorizedClient clienteAutorizado
    ) {
        // Pegamos o Access Token
        String tokenJwt = clienteAutorizado.getAccessToken().getTokenValue();

        // Fazemos a requisição para a porta 8082 repassando o token
        return restClient.get()
                .uri("http://localhost:8082/teste")
                .header("Authorization", "Bearer " + tokenJwt)
                .retrieve()
                .body(String.class);
    }

    @GetMapping("/")
    public String firstPage() {
        return "<h1>Bem vindo! 👋</h1>" +
               "<br><a href='/home'>Fazer Login</a>";
    }


    @GetMapping("/buscar-tarefas")
    public String buscarTarefasUser(
            @RegisteredOAuth2AuthorizedClient("keycloak") OAuth2AuthorizedClient clienteAutorizado
    ) {
        // Pegamos o Access Token
        String tokenJwt = clienteAutorizado.getAccessToken().getTokenValue();

        // Fazemos a requisição para a porta 8082 repassando o token
        return restClient.get()
                .uri("http://localhost:8082/minhas-tarefas")
                .header("Authorization", "Bearer " + tokenJwt)
                .retrieve()
                .body(String.class);
    }

   @GetMapping("/buscar-tarefas-admin")
    public String testarAdmin(@RegisteredOAuth2AuthorizedClient("keycloak") OAuth2AuthorizedClient client) {
        String tokenJwt = client.getAccessToken().getTokenValue();
        
        try {
            return restClient.get()
                    .uri("http://localhost:8082/admin/todas-tarefas")
                    .header("Authorization", "Bearer " + tokenJwt)
                    .retrieve()
                    .body(String.class);
                    
        } catch (HttpClientErrorException.Forbidden e) {
            return "ACESSO NEGADO (CODE:" + e.getStatusCode() +") : Você não tem permissão de Administrador para ver isso.";
        }
    }
}