package com.example.demo.config;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.http.HttpStatus;
import org.springframework.security.config.annotation.web.builders.HttpSecurity;
import org.springframework.security.config.annotation.web.configuration.EnableWebSecurity;
import org.springframework.security.oauth2.client.oidc.web.logout.OidcClientInitiatedLogoutSuccessHandler;
import org.springframework.security.oauth2.client.registration.ClientRegistrationRepository;
import org.springframework.security.oauth2.client.web.HttpSessionOAuth2AuthorizedClientRepository;
import org.springframework.security.oauth2.client.web.OAuth2AuthorizedClientRepository;
import org.springframework.security.web.SecurityFilterChain;
import org.springframework.security.web.authentication.HttpStatusEntryPoint;
import org.springframework.security.web.authentication.logout.LogoutSuccessHandler;
import org.springframework.security.web.csrf.CookieCsrfTokenRepository;
import org.springframework.security.web.csrf.CsrfFilter;
import org.springframework.security.web.csrf.CsrfTokenRequestAttributeHandler;
import org.springframework.security.web.util.matcher.RequestHeaderRequestMatcher;

import com.example.demo.controller.BffController;

@EnableWebSecurity
@Configuration
public class SecurityConfig {

    @Autowired
    private ClientRegistrationRepository clientRegistrationRepository;

    private static final Logger logger = LoggerFactory.getLogger(BffController.class);
    
    @Bean
    public OAuth2AuthorizedClientRepository authorizedClientRepository() {
        return new HttpSessionOAuth2AuthorizedClientRepository();
    }

    @Bean
    public SecurityFilterChain filterChain(HttpSecurity http) throws Exception {
        return http
        .csrf(csrf -> csrf
            .csrfTokenRepository(CookieCsrfTokenRepository.withHttpOnlyFalse())
            .csrfTokenRequestHandler(new CsrfTokenRequestAttributeHandler())
        )
        // FILTRO PERSONALIZADO QUE FORÇA O ENVIO DO COOKIE
        .addFilterAfter(new CsrfCookieFilter(), CsrfFilter.class)

        // Se for requisição do Vue, devolve 401 em vez de redirecionar
        .exceptionHandling(ex -> ex
            .defaultAuthenticationEntryPointFor(
                new HttpStatusEntryPoint(HttpStatus.UNAUTHORIZED),
                new RequestHeaderRequestMatcher("X-Requested-With", "XMLHttpRequest")
            )
        )

        .authorizeHttpRequests(req -> req
            .requestMatchers("/", "/public/**", "/login", "/favicon.ico", "/error", "/me").permitAll()
            .anyRequest().authenticated())
            .oauth2Login(auth -> auth
                .loginPage("/login")
                .defaultSuccessUrl("http://localhost:5173/tarefas", true)
                
                // FAILURE HANDLER: Captures exceptions that occur during the login handshake phase
                .failureHandler((request, response, exception) -> {
                    logger.info(">>>> OAuth2 Flow Failure. Cleaning up and redirecting to Vue...");
                    
                    // Se tiver lixo de sessão dessa aba velha, nós limpamos
                    if (request.getSession(false) != null) {
                        request.getSession(false).invalidate();
                    }
                    
                    // Redirect the browser back to the Vue app's root
                    response.sendRedirect("http://localhost:5173/");
                }))
            .logout(log -> log
                .logoutSuccessHandler(oidcLogoutSuccessHandler())
                .invalidateHttpSession(true)
                .deleteCookies("SESSION")
            ).build();
    }


    private LogoutSuccessHandler oidcLogoutSuccessHandler() {
        OidcClientInitiatedLogoutSuccessHandler oidcLogoutSuccessHandler =
                new OidcClientInitiatedLogoutSuccessHandler(this.clientRegistrationRepository);

        // Define para onde o Keycloak deve devolver o usuário após encerrar a sessão lá
        oidcLogoutSuccessHandler.setPostLogoutRedirectUri("http://localhost:5173/");

        // Se a sessão já tinha morrido, manda direto pro Vue
        oidcLogoutSuccessHandler.setDefaultTargetUrl("http://localhost:5173/");

        return oidcLogoutSuccessHandler;
    }
}