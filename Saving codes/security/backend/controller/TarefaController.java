package com.example.resource_server.controller;

import com.example.resource_server.model.Tarefa;
import com.example.resource_server.repository.TarefaRepository;
import org.springframework.security.core.annotation.AuthenticationPrincipal;
import org.springframework.security.oauth2.jwt.Jwt;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RestController;

import java.util.List;

@RestController
public class TarefaController {

    private final TarefaRepository repository;

    public TarefaController(TarefaRepository repository) {
        this.repository = repository;
    }

    // ROTA PARA USUÁRIOS COMUNS (Retorna só as tarefas do próprio usuário)
    @GetMapping("/minhas-tarefas")
    // @PreAuthorize("hasRole('USER')") // Exige a role USER
    public List<Tarefa> getMinhasTarefas(@AuthenticationPrincipal Jwt token) {
        String userId = token.getSubject();

        return repository.findByUserId(userId); 
    }

    // ROTA PARA O ADM (Retorna as tarefas de todo mundo do banco)
    @GetMapping("/admin/todas-tarefas")
    // @PreAuthorize("hasRole('ADMIN')") // Exige a role ADMIN
    public List<Tarefa> getTodasAsTarefas() {
        return repository.findAll();
    }
} 
