package com.example.backend.config; // Ajuste para o pacote correto do seu teste

import tools.jackson.core.type.TypeReference;
import tools.jackson.databind.ObjectMapper;
import jakarta.servlet.http.HttpServletRequest;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.InjectMocks;
import org.mockito.Mock;
import org.mockito.junit.jupiter.MockitoExtension;
import org.springframework.security.authorization.AuthorizationDecision;
import org.springframework.security.core.Authentication;
import org.springframework.security.core.authority.SimpleGrantedAuthority;
import org.springframework.security.web.access.intercept.RequestAuthorizationContext;

import java.io.InputStream;
import java.util.List;
import java.util.Map;
import java.util.function.Supplier;

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.when;

@ExtendWith(MockitoExtension.class)
public class RoleManagerTest {

    @Mock
    private ObjectMapper objectMapperMock;

    @InjectMocks
    private RoleManager roleManager;

    @Mock
    private RequestAuthorizationContext contextMock;

    @Mock
    private HttpServletRequest requestMock;

    @Mock
    private Authentication authenticationMock;

    @BeforeEach
    void setUp() throws Exception {
        // 1. Criamos um mapa "falso" imitando o que estaria no regras-acesso.json
        Map<String, Map<String, List<String>>> jsonFalso = Map.of(
                "/admin/todas-tarefas", Map.of("GET", List.of("ROLE_ADMIN"))
        );

        // Ensinamos o ObjectMapper a devolver o nosso mapa quando o RoleManager tentar ler o arquivo
        when(objectMapperMock.readValue(any(InputStream.class), any(TypeReference.class)))
                .thenReturn(jsonFalso);

        // Forçamos o PostConstruct a rodar para carregar as regras na memória RAM do teste
        roleManager.carregarRegras();
    }

    @Test
    @DisplayName("Deve BLOQUEAR acesso de ROLE_USER na rota de Administrador")
    void testBloquearUserNoAdmin() {
        // ARRANGE
        when(contextMock.getRequest()).thenReturn(requestMock);
        when(requestMock.getRequestURI()).thenReturn("/admin/todas-tarefas");
        when(requestMock.getMethod()).thenReturn("GET");

        when(authenticationMock.isAuthenticated()).thenReturn(true);
        
        
        doReturn(List.of(new SimpleGrantedAuthority("ROLE_USER")))
                .when(authenticationMock).getAuthorities();

        Supplier<Authentication> authSupplier = () -> authenticationMock;

        // ACT
        AuthorizationDecision decisao = roleManager.authorize(authSupplier, contextMock);

        // ASSERT
        assertFalse(decisao.isGranted(), "Falha: O sistema permitiu um USER acessar rota de ADMIN!");
    }

    @Test
    @DisplayName("Deve PERMITIR acesso de ROLE_ADMIN na rota de Administrador")
    void testPermitirAdminNoAdmin() {
        // ARRANGE
        when(contextMock.getRequest()).thenReturn(requestMock);
        when(requestMock.getRequestURI()).thenReturn("/admin/todas-tarefas");
        when(requestMock.getMethod()).thenReturn("GET");

        when(authenticationMock.isAuthenticated()).thenReturn(true);

        // Injetando a Role correta
        doReturn(List.of(new SimpleGrantedAuthority("ROLE_ADMIN")))
                .when(authenticationMock).getAuthorities();

        Supplier<Authentication> authSupplier = () -> authenticationMock;

        // ACT
        AuthorizationDecision decisao = roleManager.authorize(authSupplier, contextMock);

        // ASSERT
        assertTrue(decisao.isGranted(), "Falha: O sistema bloqueou um ADMIN legítimo!");
    }

    @Test
    @DisplayName("Deve BLOQUEAR acesso se a rota não existir no JSON")
    void testBloquearRotaInexistente() {
        // ARRANGE
        when(contextMock.getRequest()).thenReturn(requestMock);
        // Simulando tentativa de acesso a uma rota não mapeada
        when(requestMock.getRequestURI()).thenReturn("/rota-secreta-nao-mapeada");
        when(requestMock.getMethod()).thenReturn("GET");

        Supplier<Authentication> authSupplier = () -> authenticationMock;

        // ACT
        AuthorizationDecision decisao = roleManager.authorize(authSupplier, contextMock);

        // ASSERT
        assertFalse(decisao.isGranted(), "Falha: O sistema permitiu acesso a uma rota desconhecida!");
    }
}