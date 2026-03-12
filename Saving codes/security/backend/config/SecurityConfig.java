package com.example.resource_server.config;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.security.config.annotation.method.configuration.EnableMethodSecurity;
import org.springframework.security.config.annotation.web.builders.HttpSecurity;
import org.springframework.security.config.annotation.web.configuration.EnableWebSecurity;
import org.springframework.security.config.http.SessionCreationPolicy;
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
    public SecurityFilterChain filterChain(HttpSecurity http, JwtConfig jwtConfig, JwtAuthenticationConverter roleCatcher, RoleManager roleManager) throws Exception {
        http
            .csrf(csrf -> csrf.disable()) // Desnecessário para API Rest Stateless
            
            .sessionManagement(session -> session
                .sessionCreationPolicy(SessionCreationPolicy.STATELESS) // Nao guarda estado
            )
            
            .authorizeHttpRequests(authorize -> authorize
                .anyRequest().access(roleManager) // Any request goes thru the Role filter
            )
            
            .oauth2ResourceServer(oauth2 -> oauth2
                .jwt(jwt -> jwt
                    .decoder(jwtConfig.identifyJwtType()) 
                    .jwtAuthenticationConverter(roleCatcher) 
                ) 
            );

        return http.build();
    }
}