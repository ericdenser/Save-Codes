package com.example.demo.config;

import org.springframework.security.web.csrf.CsrfToken;
import org.springframework.web.filter.OncePerRequestFilter;

import jakarta.servlet.FilterChain;
import jakarta.servlet.ServletException;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;
import java.io.IOException;

public class CsrfCookieFilter extends OncePerRequestFilter {

    @Override
    protected void doFilterInternal(HttpServletRequest request, HttpServletResponse response, FilterChain filterChain)
            throws ServletException, IOException {
        
        // Pega o token CSRF que o Spring preparou 
        CsrfToken csrfToken = (CsrfToken) request.getAttribute(CsrfToken.class.getName());
        
        if (csrfToken != null) {
            // chamar o .getToken() acorda o Spring e força ele a gerar o Cookie
            csrfToken.getToken();
        }
        
        // Segue o fluxo normal da requisição
        filterChain.doFilter(request, response);
    }
}