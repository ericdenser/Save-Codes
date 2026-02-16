package com.example.backend.config;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.security.config.annotation.web.builders.HttpSecurity;
import org.springframework.security.config.annotation.web.configuration.EnableWebSecurity;
import org.springframework.security.config.http.SessionCreationPolicy;
import org.springframework.security.oauth2.core.DelegatingOAuth2TokenValidator;
import org.springframework.security.oauth2.core.OAuth2Error;
import org.springframework.security.oauth2.core.OAuth2TokenValidator;
import org.springframework.security.oauth2.core.OAuth2TokenValidatorResult;
import org.springframework.security.oauth2.jwt.*;
import org.springframework.security.web.SecurityFilterChain;

@Configuration
@EnableWebSecurity
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
                // Avisamos o Spring para usar o nosso decodificador customizado e nao o padrão
                .jwt(jwt -> jwt.decoder(jwtDecoder())) 
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
}