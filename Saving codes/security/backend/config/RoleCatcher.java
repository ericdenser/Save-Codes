package com.example.backend.config;

import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.security.core.authority.SimpleGrantedAuthority;
import org.springframework.security.oauth2.server.resource.authentication.JwtAuthenticationConverter;

@Configuration
public class RoleCatcher {

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