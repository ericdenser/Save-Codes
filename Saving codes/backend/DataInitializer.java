package com.example.backend;

import com.example.backend.model.Tarefa;
import com.example.backend.repository.TarefaRepository;
import org.springframework.boot.CommandLineRunner;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

@Configuration
public class DataInitializer {

    @Bean
    public CommandLineRunner carregarDados(TarefaRepository repository) {
        return args -> {
            if (repository.count() == 0) {
                // Tarefas do seu usuário
                repository.save(new Tarefa("Aprender Vue.js", "288c0eca-bcaa-4064-9512-8db4ed462f3c"));
                repository.save(new Tarefa("Dominar Spring Security", "4233840e-0d04-4ccd-aca6-551b696064c8"));
                
                // Tarefas de outro usuário
                repository.save(new Tarefa("Plano de Dominação Global", "hacker_do_mal"));
                
                System.out.println("Dados fictícios carregados com sucesso!");
            }
        };
    }
}