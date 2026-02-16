package com.example.bff.controller;

import java.util.Map;

import org.springframework.security.core.annotation.AuthenticationPrincipal;
import org.springframework.security.oauth2.client.OAuth2AuthorizedClient;
import org.springframework.security.oauth2.client.annotation.RegisteredOAuth2AuthorizedClient;
import org.springframework.security.oauth2.core.oidc.user.OidcUser;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.client.RestClient;

@RestController
public class BffController {

    private final RestClient restClient;

    public BffController(RestClient restClient) {
        this.restClient = restClient;
    }

    @GetMapping("/home")
    public String home(@AuthenticationPrincipal OidcUser principal) {
        return "Olá, " + principal.getFullName() + 
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

    @GetMapping("/backend")
    public String buscarTarefasNoBackend(
            // Essa anotação mágica faz o Spring ir no Redis e pegar os tokens do usuário atual!
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
}