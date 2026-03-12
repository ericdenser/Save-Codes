package com.example.demo.config;
 
import org.springframework.http.HttpRequest;
import org.springframework.http.client.ClientHttpRequestExecution;
import org.springframework.http.client.ClientHttpRequestInterceptor;
import org.springframework.http.client.ClientHttpResponse;
import org.springframework.security.core.Authentication;
import org.springframework.security.core.context.SecurityContextHolder;
import org.springframework.security.oauth2.client.OAuth2AuthorizeRequest;
import org.springframework.security.oauth2.client.OAuth2AuthorizedClient;
import org.springframework.security.oauth2.client.OAuth2AuthorizedClientManager;
import org.springframework.security.oauth2.client.authentication.OAuth2AuthenticationToken;
 
import java.io.IOException;
 
public class TokenRelayInterceptor implements ClientHttpRequestInterceptor {
 
    private final OAuth2AuthorizedClientManager authorizedClientManager;
 
    public TokenRelayInterceptor(OAuth2AuthorizedClientManager authorizedClientManager) {
        this.authorizedClientManager = authorizedClientManager;
    }
 
    @Override
    public ClientHttpResponse intercept(HttpRequest request, byte[] body, ClientHttpRequestExecution execution) throws IOException {
        // Quem é que está tentando fazer essa chamada?
        Authentication authentication = SecurityContextHolder.getContext().getAuthentication();
 
        // Se for um usuário logado via OAuth2
        if (authentication instanceof OAuth2AuthenticationToken oauthToken) {
            
            // Monta um pedido para o Gerente: "Me dá o token válido para este cliente"
            OAuth2AuthorizeRequest authorizeRequest = OAuth2AuthorizeRequest
                    .withClientRegistrationId(oauthToken.getAuthorizedClientRegistrationId())
                    .principal(authentication)
                    .build();
 
            // O Gerente verifica se o token existe e se não expirou (renova se precisar)
            OAuth2AuthorizedClient authorizedClient = authorizedClientManager.authorize(authorizeRequest);
 
            // Se conseguiu o token, coloca no Header Authorization
            if (authorizedClient != null && authorizedClient.getAccessToken() != null) {
                request.getHeaders().setBearerAuth(authorizedClient.getAccessToken().getTokenValue());
            }
        }
 
        // Segue o fluxo 
        return execution.execute(request, body);
    }
}
 