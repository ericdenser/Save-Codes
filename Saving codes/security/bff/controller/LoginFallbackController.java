package com.example.bff.controller;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Controller;
import org.springframework.web.bind.annotation.GetMapping;


@Controller
public class LoginFallbackController {

    private static final Logger logger = LoggerFactory.getLogger(BffController.class);

    @GetMapping("/login")
    public String redirecionarLoginPerdido() {
        logger.info("Requisição perdida no /login interceptada. Devolvendo para o Vue...");
        
        // A palavra "redirect:" faz o Spring devolver um HTTP 302 para o navegador do usuário
        return "redirect:http://localhost:5173/";
    }
}