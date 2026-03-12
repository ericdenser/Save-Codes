package com.example.demo.service;

import java.util.List;
import java.util.Map;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.security.oauth2.client.OAuth2AuthorizeRequest;
import org.springframework.security.oauth2.client.OAuth2AuthorizedClient;
import org.springframework.security.oauth2.client.OAuth2AuthorizedClientManager;
import org.springframework.security.oauth2.client.authentication.OAuth2AuthenticationToken;
import org.springframework.security.oauth2.core.oidc.user.OidcUser;
import org.springframework.stereotype.Service;

import com.nimbusds.jwt.SignedJWT;

import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;

@Service
public class UserService {

    private static final Logger logger = LoggerFactory.getLogger(UserService.class);

    // Injecting Spring's intelligent OAuth2 Manager to handle token lifecycle
    private final OAuth2AuthorizedClientManager clientManager;

    public UserService(OAuth2AuthorizedClientManager clientManager) {
        this.clientManager = clientManager;
    }

   // Helper method to check is the user is authorized
    private OAuth2AuthorizedClient getAuthorizedClient(
            OAuth2AuthenticationToken authToken, 
            HttpServletRequest request, 
            HttpServletResponse response) {
        
         /*
         * PREPARING THE AUTHORIZATION REQUEST
         * - We build a request with all the context Spring Security needs to find and manage our tokens.
         * - withClientRegistrationId: Tells Spring which Identity Provider we are dealing with ("keycloak").
         * - principal: Identifies the current user. Spring uses this name to find them in Redis.
         * - attributes (request/response): Since our tokens are tied to the Session cookie and stored in Redis,
         * Spring needs the HttpServletRequest to read the incoming cookie, and the HttpServletResponse to overwrite/save 
         * new tokens in Redis if a refresh happens. Without these, the manager is "blind" and cannot read/write sessions.
         */
        OAuth2AuthorizeRequest authorizeRequest = OAuth2AuthorizeRequest
                .withClientRegistrationId(authToken.getAuthorizedClientRegistrationId())
                .principal(authToken)
                .attribute(HttpServletRequest.class.getName(), request)
                .attribute(HttpServletResponse.class.getName(), response)
                .build();


        /*
         * EXECUTING TOKEN RESOLUTION & REFRESH
         * This single method:
         * - Goes to Redis (using the request context) to fetch the saved Access and Refresh Tokens.
         * - Checks the Access Token's expiration time (exp).
         * - If the token is valid, it simply returns it.
         * - If the token is expired, it pauses, makes a silent HTTP call to Keycloak's 
         * /token endpoint using the Refresh Token, gets new tokens, saves them back to Redis (using the response context), 
         * and then returns the fresh, valid tokens.
         */
        return clientManager.authorize(authorizeRequest);
    }


    // Method responsible for returning the current authenticaded user Info 
    public Map<String, Object> getAuthenticatedUserInfo(
            OAuth2AuthenticationToken authToken, 
            OidcUser principal, 
            HttpServletRequest request, 
            HttpServletResponse response) {

        // Helper to check if user is valid
        OAuth2AuthorizedClient client = getAuthorizedClient(authToken, request, response);


        // If the client's session in Redis is gone or invalid
        if (client == null || client.getAccessToken() == null) {
            logger.warn("Tokens não encontrados. Retornando null.");
            return null; 
        }

        logger.info("UserService /me ativado, nome: {}", principal.getFullName());
        
        // Builds and returns a clean Map with user data for the frontend
        return Map.of(
            "authenticated", true,
            "nome", principal.getFullName(),
            "username", principal.getPreferredUsername()
        );
    }


    // Method to check if user roles match the specific role passed in the parameter
    public boolean checkUserHasRole(
            List<String> requiredRoles,
            OAuth2AuthenticationToken authToken, 
            HttpServletRequest request, 
            HttpServletResponse response) {

        // Call our helper
        OAuth2AuthorizedClient client = getAuthorizedClient(authToken, request, response);

        if (client == null || client.getAccessToken() == null) {
            return false; 
        }

        return hasRoleInJwt(client, requiredRoles); 
    }

    // Private method to return if user has the required role
    private boolean hasRoleInJwt(OAuth2AuthorizedClient client, List<String> requiredRoles) {
        try {

            // Get the actual Access Token string currently stored in the BFF (Redis)
            String accessTokenStr = client.getAccessToken().getTokenValue();

            // Parse the Token (Decode the JWT payload using the Nimbus library)
            SignedJWT jwt = SignedJWT.parse(accessTokenStr);

            // Extract the "realm_access" claim block (where Keycloak hides the user roles)
            @SuppressWarnings("unchecked")
            Map<String, Object> realmAccess = (Map<String, Object>) jwt.getJWTClaimsSet().getClaim("realm_access");

            // If roles exist
            if (realmAccess != null && realmAccess.containsKey("roles")) {
                @SuppressWarnings("unchecked")
                // extract them
                List<String> roles = (List<String>) realmAccess.get("roles");

                // append the "ROLE_" prefix required by Spring Security and Vue Router
                List<String> formattedRoles = roles.stream()
                        .map(role -> "ROLE_" + role.toUpperCase())
                        .toList();
                logger.info("Roles do usuário no token: " + formattedRoles);
                logger.info("Roles que podem acessar: " + requiredRoles);
                
                // Returns if user roles match any of the required roles
                return formattedRoles.stream()
                    .anyMatch(requiredRoles::contains);
            }
        } catch (Exception e) {
            logger.error("Error when verifying user roles in JWT: {}", e.getMessage());
        }
        return false;
    }
}
