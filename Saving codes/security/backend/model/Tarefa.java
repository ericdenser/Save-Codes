package com.example.backend.model;

import jakarta.persistence.*;

@Entity
public class Tarefa {
    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;
    
    private String titulo;
    
    private String userId; 

    public Tarefa() {}
    
    public Tarefa(String titulo, String userId) {
        this.titulo = titulo;
        this.userId = userId;
    }

    // Gere os Getters e Setters!
    public Long getId() { return id; }
    public String getTitulo() { return titulo; }
    public String getUserId() { return userId; }
}