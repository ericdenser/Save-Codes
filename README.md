comandos redis:

docker exec -it "id" redis-cli

keys *

hkeys "id"

ttl "id"

comandos postgre:
---
docker exec -it security-postgres-1 psql -U keycloak (abrir db)
\l (listar)
\q (sair)
---
docker exec -it "id" psql -U keycloak -c "CREATE DATABASE resource_db;" (cria db)

INFOS QUE NAO PODE ESQUECER:

1. usar sub para indentificar usuario (uuid)
2. jwt do redis so muda quando expira o token
3. adicao de conversor de role e controle de token na filterchain do backend
4. "/favicon.ico", "/error" no permitAll, pois o navegador pede, mas o spring nao encontra o favicon (erro 404, tenta redirecionar internamente para a rota /error mas a /error é rota privada, que força o keycloak a relogar e ficar criando tokens novos)


1. spring initalzr (Spring web, Oauth2 Server)
2. `POM`:

<dependency>
    <groupId>org.springframework.boot</groupId>
    <artifactId>spring-boot-starter-web</artifactId>
</dependency>
<dependency>
    <groupId>org.springframework.boot</groupId>
    <artifactId>spring-boot-starter-oauth2-resource-server</artifactId>
</dependency>

3. `YAML: `

server:
  port: 8082  # Porta diferente do BFF (8081) e Keycloak (8080)

spring:
  application:
    name: resource-server
  security:
    oauth2:
      resourceserver:
        jwt:
          # O Spring vai bater nessa URL ao iniciar para baixar as chaves públicas (JWK Set)
          # Ele valida se o token foi assinado por ESSE emissor.
          issuer-uri: http://localhost:8080/realms/bff-spring



4. package com.example.backend.config;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.security.config.annotation.web.builders.HttpSecurity;
import org.springframework.security.config.annotation.web.configuration.EnableWebSecurity;
import org.springframework.security.config.http.SessionCreationPolicy;
import org.springframework.security.web.SecurityFilterChain;

@Configuration
@EnableWebSecurity
public class SecurityConfig {

    @Bean
    public SecurityFilterChain filterChain(HttpSecurity http) throws Exception {
        http
            //Desnecessário para APIs Rest Stateless
            .csrf(csrf -> csrf.disable())

            // Define que NÃO haverá sessão (Não guarda estado no servidor)
            .sessionManagement(session -> session
                .sessionCreationPolicy(SessionCreationPolicy.STATELESS)
            )

            //Regras de acesso
            .authorizeHttpRequests(authorize -> authorize
                .anyRequest().authenticated() // Tudo precisa de token
            )

            //Habilita validação de Token JWT
            .oauth2ResourceServer(oauth2 -> oauth2
                .jwt(jwt -> {}) // Usa o padrão do Spring (valida assinatura e expiração)
            );

        return http.build();
    }
}

5. package com.example.backend.controller;

import org.springframework.security.core.annotation.AuthenticationPrincipal;
import org.springframework.security.oauth2.jwt.Jwt;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RestController;

import java.util.Map;

@RestController
public class TesteController {

    @GetMapping("/teste")
    public Map<String, String> listarTarefas(@AuthenticationPrincipal Jwt token) {
        
        String usuario = token.getClaim("preferred_username");
        String email = token.getClaim("email");

        return Map.of(
            "status", "Sucesso! Você acessou o Backend.",
            "usuario", usuario,
            "email", email,
            "token_id", token.getId()
        );
    }
}package com.example.backend.controller;

import org.springframework.security.core.annotation.AuthenticationPrincipal;
import org.springframework.security.oauth2.jwt.Jwt;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RestController;

import java.util.Map;

@RestController
public class TesteController {

    @GetMapping("/teste")
    public Map<String, String> listarTarefas(@AuthenticationPrincipal Jwt token) {
        
        String usuario = token.getClaim("preferred_username");
        String email = token.getClaim("email");

        return Map.of(
            "status", "Sucesso! Você acessou o Backend.",
            "usuario", usuario,
            "email", email,
            "token_id", token.getId()
        );
    }
}

# NO BFF:
6.

package com.example.bff.config;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.web.client.RestClient;

@Configuration
public class WebClientConfig {

    @Bean
    public RestClient.Builder restClientBuilder() {
        return RestClient.builder();
    }


    @Bean
    public RestClient restClient(RestClient.Builder builder) {
        
        return builder.build();
    }
}

7. @GetMapping("/backend")
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

# CONFIGURANDO LOGOUT

