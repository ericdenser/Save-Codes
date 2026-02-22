package com.example.backend.config;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.security.oauth2.core.DelegatingOAuth2TokenValidator;
import org.springframework.security.oauth2.core.OAuth2Error;
import org.springframework.security.oauth2.core.OAuth2TokenValidator;
import org.springframework.security.oauth2.core.OAuth2TokenValidatorResult;
import org.springframework.security.oauth2.jwt.*;

@Configuration
public class JwtConfig {

    @Value("${spring.security.oauth2.resourceserver.jwt.issuer-uri}")
    private String issuerUri;

    @Bean
    public JwtDecoder jwtDecoder() {
        // Cria o decodificador base (conectando no Keycloak para pegar a chave pública)
        NimbusJwtDecoder jwtDecoder = JwtDecoders.fromIssuerLocation(issuerUri);

         // Mantém as validacões padrao de segurança do Spring (Data de expiracao e Emissor)
        OAuth2TokenValidator<Jwt> CommonValidators = JwtValidators.createDefaultWithIssuer(issuerUri);

        // Cria a validacão adicional ( O token tem que ser um Access Token (typ = Bearer))
        OAuth2TokenValidator<Jwt> tokenTypeValidator = jwt -> {
            String tokenType = jwt.getClaimAsString("typ");
            if ("Bearer".equals(tokenType)) {
                return OAuth2TokenValidatorResult.success();
            }

            // Rejeita qualquer outro tipo de token mesmo que a matematica de decodificacao bata
            return OAuth2TokenValidatorResult.failure(new OAuth2Error(
                    "invalid_token",
                    "Access Denied: The token is not valid (Not Access Token). Type: " + tokenType,
                    null
            ));
        };

        // Combina as validacoes (padrao e de token)
        OAuth2TokenValidator<Jwt> combinedValidators = new DelegatingOAuth2TokenValidator<>(
                CommonValidators,
                tokenTypeValidator
        );
        // Aplica no fluxo de decodificacao
        jwtDecoder.setJwtValidator(combinedValidators);
        return jwtDecoder;
    }
}
