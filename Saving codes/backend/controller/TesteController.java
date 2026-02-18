package com.example.backend.controller;

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