# 🛡️ Enterprise Security Architecture: Vue.js + Spring Boot BFF + Keycloak

![Vue.js](https://img.shields.io/badge/Vue.js-3.0-42b983?style=flat-square&logo=vue.js)
![Spring Boot](https://img.shields.io/badge/Spring_Boot-3.x-6DB33F?style=flat-square&logo=spring)
![Keycloak](https://img.shields.io/badge/Keycloak-OIDC-0097CE?style=flat-square&logo=keycloak)
![Redis](https://img.shields.io/badge/Redis-Session-DC382D?style=flat-square&logo=redis)

Um projeto de arquitetura de alta segurança projetado para resolver os problemas modernos de Single Page Applications (SPAs), implementando o padrão **BFF (Backend For Frontend)** em conformidade com as melhores práticas de cibersegurança do mercado (OWASP).

## 🏛️ Arquitetura do Sistema

A aplicação é dividida em 4 camadas isoladas, garantindo que o navegador do usuário nunca tenha acesso a tokens JWT sensíveis.

1. **Frontend (Vue.js 3 + Pinia + Axios):** SPA "burra" e stateless. Não armazena tokens no `localStorage`. Opera apenas via cookies de sessão opacos e interceptadores de segurança globais.
2. **BFF - Backend For Frontend (Spring Boot 8081):** O maestro da segurança. Gerencia o fluxo OAuth2/OIDC, guarda os Tokens JWTs com segurança, traduz sessões e atua como Proxy Reverso para o backend real.
3. **Identity Provider (Keycloak 8080):** Servidor de autorização, responsável por gerenciar usuários, políticas de tempo de sessão e fornecer Access/Refresh Tokens.
4. **Resource Server (Spring Boot 8082):** A API de dados real. Totalmente *Stateless*. Analisa assinaturas criptográficas dos JWTs vindos do BFF e aplica Controle de Acesso Baseado em Regras (RBAC).
5. **Session Cache (Redis):** Armazenamento de sessão distribuída para o BFF, garantindo escalabilidade horizontal.

---

## 🔒 Mecanismos de Segurança e Hardening Implementados

Este projeto foi construído para mitigar as vulnerabilidades mais críticas do desenvolvimento web moderno:

* **Ocultação de Token (Mitigação de XSS):** Adoção estrita do padrão BFF. O Access Token e o Refresh Token residem apenas no Redis/BFF. O navegador recebe apenas um cookie `SESSION` com a flag `HttpOnly`.
* **Proteção CSRF (Double Submit Cookie):** O BFF envia um cookie `XSRF-TOKEN` legível pelo frontend. O Axios intercepta nativamente e o devolve em requisições de mutação (POST/PUT/DELETE), garantindo que a requisição partiu do próprio site.
* **Prevenção de Abas Fantasmas e Replay Attacks:** Implementação de cabeçalhos de controle `X-Requested-With: XMLHttpRequest` para diferenciar navegação humana de chamadas AJAX, evitando loops de redirecionamento 302 em sessões expiradas.
* **Gerenciamento de Exceções OIDC:** Interceptadores globais (`@ControllerAdvice`) que capturam erros de `invalid_grant` do Keycloak, limpam sessões zumbis no Redis e forçam redirecionamentos controlados (Status 401) no frontend.
* **Prevenção de State Mismatch (OAuth2 CSRF):** Customização do `AuthenticationFailureHandler` no Spring Security para proteger contra conflitos de sessão causados por autenticações em múltiplas abas concorrentes.
* **Security Headers Ativos:** Injeção de `X-Frame-Options: DENY` e `Content-Security-Policy (CSP)` para proteger o Vue.js contra ataques de Clickjacking e injeção de iframes de terceiros.

---

## 🛠️ RBAC Dinâmico (Role-Based Access Control) em Tempo Constante O(1)

O controle de autorização do Resource Server (8082) não depende de anotações estáticas hardcoded (como `@PreAuthorize`). 
Foi implementado um **RoleManager dinâmico** utilizando `AuthorizationManager`.

* **Arquivo JSON de Políticas:** As regras de negócio ficam em um arquivo `regras-acesso.json` isolado do código-fonte.
* **Eficiência:** O JSON é carregado no `@PostConstruct` do Spring e mapeado para a memória RAM (HashMaps), permitindo verificações de segurança em tempo constante $O(1)$ sem impactar a latência da API.
* **Garantia de Qualidade:** O mecanismo de RBAC é validado por suítes de **Testes Unitários utilizando JUnit 5 e Mockito**, cobrindo matrizes de decisão para usuários comuns (`ROLE_USER`), administradores (`ROLE_ADMIN`) e rotas inexistentes.

---

## 🚀 Como executar localmente

1. **Subir a infraestrutura base:** (Requer Docker)
   Inicie o Keycloak e o Redis utilizando o `docker-compose`.
2. **Configurar Keycloak:**
   Importe o arquivo `realm-export.json` (se disponível) para carregar os clientes e usuários pré-configurados.
3. **Iniciar os Backends:**
   * Rode o Resource Server (Porta 8082).
   * Rode o BFF (Porta 8081).
4. **Iniciar o Frontend:**
   * Entre na pasta do Vue.js, rode `npm install` e em seguida `npm run dev`.
   * Acesse `http://localhost:5173`.
