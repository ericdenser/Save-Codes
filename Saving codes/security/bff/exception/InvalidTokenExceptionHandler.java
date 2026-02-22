package com.example.bff.exception;

import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpSession;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.security.oauth2.client.ClientAuthorizationException;
import org.springframework.web.bind.annotation.ControllerAdvice;
import org.springframework.web.bind.annotation.ExceptionHandler;

import com.example.bff.controller.BffController;

import java.util.Map;

@ControllerAdvice
public class InvalidTokenExceptionHandler {

    private static final Logger logger = LoggerFactory.getLogger(BffController.class);

    // Fica escutando qualquer erro de autorização de cliente OAuth2 que o Spring lançar
    @ExceptionHandler(ClientAuthorizationException.class)
    public ResponseEntity<Map<String, String>> handleTokenRefreshError(ClientAuthorizationException ex, HttpServletRequest request) {
        
        // Se o erro for "invalid_grant", o Refresh Token morreu no Keycloak.
        if ("invalid_grant".equals(ex.getError().getErrorCode())) {
            logger.info("🚨 [SECURITY] Refresh Token rejeitado pelo Keycloak. Limpando a sessão do Redis...");
            
            // Mata a sessão no Redis
            HttpSession session = request.getSession(false);
            if (session != null) {
                session.invalidate();
            }
            
            //Devolve o 401 pro Axios no Vue -> Hard Reset
            return ResponseEntity.status(HttpStatus.UNAUTHORIZED)
                                 .body(Map.of("erro", "Sessão expirada no servidor de autenticação. Faça login novamente."));
        }
        
        // Se for outro erro bizarro, mantemos o erro 500 para investigarmos
        return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR)
                             .body(Map.of("erro", "Erro interno de Autenticação: " + ex.getMessage()));
    }
}