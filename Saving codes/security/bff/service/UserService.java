package com.example.bff.service;

import java.util.List;
import java.util.Map;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.security.oauth2.client.OAuth2AuthorizedClient;
import org.springframework.stereotype.Service;

import com.nimbusds.jwt.SignedJWT;

@Service
public class UserService {

    private static final Logger logger = LoggerFactory.getLogger(UserService.class);
    
    public UserService() {}

    
    public List<String> getUserRoles(OAuth2AuthorizedClient client) {

        try {
            // 1. Pega o Access Token real que o BFF está guardando
            String accessTokenStr = client.getAccessToken().getTokenValue();

            // 2. Abre o Token (Decodifica o payload)
            SignedJWT jwt = SignedJWT.parse(accessTokenStr);
            
            // 3. Extrai o bloco "realm_access"
            Map<String, Object> realmAccess = (Map<String, Object>) jwt.getJWTClaimsSet().getClaim("realm_access");

            // 4. Se tiver roles, extraímos e formatamos com o "ROLE_"
            if (realmAccess != null && realmAccess.containsKey("roles")) {
                List<String> roles = (List<String>) realmAccess.get("roles");
                
                return roles.stream()
                        .map(role -> "ROLE_" + role.toUpperCase())
                        .toList();
            }
        } catch (Exception e) {
            logger.info("Erro ao extrair roles do Access Token no BFF: " + e.getMessage());
        }
        return List.of();
    }
}
