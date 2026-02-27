# Protecting the API with Authorization

In this tutorial, we will teach you how to protect your API with Authorization.

> [!Important] This guide continues from a previous tutorial. 
> We use the same project we finished from last tutorial. If your application is missing something, your tests might fail. So before proceding with the implementations, make sure you followed all the steps presented on the [Protecting your API with Authentication](LearningPaths/ArchitectureBFFandKeycloak/ProtectingApi_Authentication.md). 

# Prerequisites

1. VsCode installed ([Windows tutorial](ide/InstallVSCodeInWindows.md), [Debian tutorial](ide/InstallVSCode-Debian.md))
2. Java and Maven configured in your development environment. [Tutorials here](https://mackcloud.mackenzie.br/gitlab/fci/learning/-/tree/main/java?ref_type=heads)
3. API implemented with simple controller (This tutorial will use the same API developed on the [MyFirstJavaRestAPI tutorial](https://mackcloud.mackenzie.br/gitlab/fci/learning/-/blob/main/LearningPaths/MyFirstJavaRestAPI.md?ref_type=heads))
4. BFF implemented with Keycloak and Redis [Tutorial]()
5. API implemented with Authentication [Tutorial]()
--- 

# Protecting the API

## Authorization explaining

In the last tutorial, you learned and implemented Authentication. While Authentication is about verifying who you are, Authorization is about determining what you are allowed to do.

Authorization is one of the most important safety layers on any application. You can block, allow and limit what the user access. Basically every user will have a ROLE, and that ROLE is used on every request that the API can verify the user's permissions. Based on your business logic, the server decides whether to grant access to the requested resource or block it.

## Role-Based Access Control (RBAC)

Managing permissions user by user is impossible to maintain. To resolve this, we use RBAC (Role-Based Access Control). Instead of assigning permissions directly to individuals, we assign them to Roles, and then assign those roles to users.

---

## Implementation

There are many ways of implementing Authorization on your system. Spring Boot offers quick and easy tools to activate Authorization, that we only use on the first test to verify if the ROLE is being assigned. For the final implementation, we present you a more flexible and practical strategy than the first one.

Lest start.

### Creating Role on Keycloak

1. Open the Keycloak admin interface.
2. Select the Realm of your application.
3. On the left navigation bar, click "Realm Roles".

It will lead you to this page:

![Image01](LearningPaths/images/_protectingApi__keycloak_roles.png)

4. Click on "Create Role"
5. It will open a window asking the "Role Name" and "Description", name it as ADMIN(optional) 
6. Click on save and the Role is successfully created.

### Assigning the Role to a User

1. Still on the Keycloak admin interface, on the left navigation bar, click on "Users".
2. It will open a window of all the created Users on your realm. 
3. Select a user to assign the Role (Use an existing one or Create a new User)
4. Click on Role Mapping -> Assign Role 

It opens this page, click on "Filter by clients" and change for "Filter by realm role"

![Image03](LearningPaths/images/_protectingApi__keycloak_assign_role.png)

5. It will open a list of roles, select the one you created -> Assign.

### Applying the Role on Spring

Now that we have a Role created and assigned to a User, we need to apply the Authorization strategy on the API. Before we write the code, we need to understand a small conflict between Keycloak and Spring Security.

Keycloak stores our roles inside Access Token. If you inspect the token on jwt.io, you will find this:

```json
  "realm_access": {

    "roles": [

      "default-roles-bff-spring",

      "offline_access",

      "USER",

      "uma_authorization",

      "ADMIN"

    ]

  },
```

We need to teach Spring how to extract this specific data inside the JWT and convert it into standard Spring Authorities (Roles).

#### Creating the Role Converter

To fix this, we will create a custom converter. Create a new class called `KeycloakRoleConverte` inside config folder of your API.

```java
package com.example.resource_server.config;

import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.security.core.authority.SimpleGrantedAuthority;
import org.springframework.security.oauth2.server.resource.authentication.JwtAuthenticationConverter;

@Configuration
public class RoleCatcher {

    @Bean
    public JwtAuthenticationConverter conversorDeRolesDoKeycloak() {
        JwtAuthenticationConverter converter = new JwtAuthenticationConverter();
        
        converter.setJwtGrantedAuthoritiesConverter(jwt -> {
            // map realm_acces
            Map<String, Object> realmAccess = jwt.getClaim("realm_access");
            
            // if its null or doesnt contain any role
            if (realmAccess == null || !realmAccess.containsKey("roles")) {
                return List.of(); // return empty
            }
            
            // Extracts the Role list
            List<String> rolesDoKeycloak = (List<String>) realmAccess.get("roles");
            
            // Map each role to the current user with the prefix "ROLE_" 
            return rolesDoKeycloak.stream()
                    .map(roleName -> new SimpleGrantedAuthority("ROLE_" + roleName.toUpperCase()))
                    .collect(Collectors.toList());
        });
        
        return converter;
    }
}
```

Lets apply it on our FilterChain.  
Open your `SecurityConfig` class and make the same changes below:

```java
package com.example.resource_server.config;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.security.config.annotation.method.configuration.EnableMethodSecurity;
import org.springframework.security.config.annotation.web.builders.HttpSecurity;
import org.springframework.security.config.annotation.web.configuration.EnableWebSecurity;
import org.springframework.security.config.http.SessionCreationPolicy;
import org.springframework.security.oauth2.jwt.*;
import org.springframework.security.web.SecurityFilterChain;

@Configuration
@EnableWebSecurity 
@EnableMethodSecurity // This allows us to use authorization annotations directly on our controllers
public class SecurityConfig {

    @Value("${spring.security.oauth2.resourceserver.jwt.issuer-uri}")
    private String issuerUri;

    @Bean
    public SecurityFilterChain filterChain(HttpSecurity http, JwtDecoder jwtDecoder, JwtAuthenticationConverter roleCatcher) throws Exception {
        http
            .csrf(csrf -> csrf.disable()) 
            
            .sessionManagement(session -> session.sessionCreationPolicy(SessionCreationPolicy.STATELESS)) 
            .authorizeHttpRequests(authorize -> authorize.anyRequest().authenticated()) // Any request must be authenticated

            .oauth2ResourceServer(oauth2 -> oauth2
                .jwt(jwt -> jwt
                    .decoder(jwtDecoder) // our customized decoder to check token type
                    .jwtAuthenticationConverter(roleCatcher)  // OUR NEW CLASS 
                ) 
            );

        return http.build();
    }

}
```
Our API is now reading the ROLE from JWT and linking to the current User. Lets test it blocking some endpoints. 

#### Testing

Create 2 new methods inside your Controller, one will be avaiable for your role, and the other dont. 

```java
    // Allowed for USERS and ADMINS
    @GetMapping("/avatar")
    @PreAuthorize("hasAnyRole('USER', 'ADMIN')")
    public List<String> listAvatar() {
        return List.of("Obi wan", "Padawan", "Darth Vader");
    }

    // Allowed only for ADMIN 
    @GetMapping("/admin")
    @PreAuthorize("hasRole('ADMIN')") 
    public String adminPage() {
        return "You have ADMIN role, welcome sir!";
    }
```




--- 

## Connecting BFF And API

On this step, we finally connect our BFF requests to our API, basically the same thing we were already testing on ThunderClient, but now the BFF will take over.

First thing we have to create is our `RestClient` configuration class, so then our BFF server can make requests to the API. Open your BFF and copy/paste the class below on the `config` folder:

```java
package com.example.demo.config;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.security.oauth2.client.OAuth2AuthorizedClientManager;
import org.springframework.web.client.RestClient;

@Configuration
public class RestClientConfig {

    @Bean
    public RestClient restClient(OAuth2AuthorizedClientManager authorizedClientManager) {
        
        return RestClient.builder()
            .requestInterceptor(new TokenRelayInterceptor(authorizedClientManager))
            .build();
             
    }
}
``` 

- OAuth2AuthorizedClientManager: This is the core Spring Security component responsible for managing our tokens.

- TokenRelayInterceptor: This is a custom request interceptor that acts as a middleman. Before any request actually leaves our BFF to hit the API, this interceptor asks the manager for the current valid Access Token, and inject it into the header, just like we were doing on the ThunderClient.


To implement that custom Token Interceptor, create this class on the `config` paste of your BFF project:

```java 
package com.example.demo.config;
 
import org.springframework.http.HttpRequest;
import org.springframework.http.client.ClientHttpRequestExecution;
import org.springframework.http.client.ClientHttpRequestInterceptor;
import org.springframework.http.client.ClientHttpResponse;
import org.springframework.security.core.Authentication;
import org.springframework.security.core.context.SecurityContextHolder;
import org.springframework.security.oauth2.client.OAuth2AuthorizeRequest;
import org.springframework.security.oauth2.client.OAuth2AuthorizedClient;
import org.springframework.security.oauth2.client.OAuth2AuthorizedClientManager;
import org.springframework.security.oauth2.client.authentication.OAuth2AuthenticationToken;
 
import java.io.IOException;
 
public class TokenRelayInterceptor implements ClientHttpRequestInterceptor {
 
    private final OAuth2AuthorizedClientManager authorizedClientManager;
 
    public TokenRelayInterceptor(OAuth2AuthorizedClientManager authorizedClientManager) {
        this.authorizedClientManager = authorizedClientManager;
    }
 
    @Override
    public ClientHttpResponse intercept(HttpRequest request, byte[] body, ClientHttpRequestExecution execution) throws IOException {
        // Who is calling
        Authentication authentication = SecurityContextHolder.getContext().getAuthentication();
 
        // Is a user authenticated via OAuth2?
        if (authentication instanceof OAuth2AuthenticationToken oauthToken) {
            
            // Builds an authorization request asking the Manager for a valid token for this specific client
            OAuth2AuthorizeRequest authorizeRequest = OAuth2AuthorizeRequest
                    .withClientRegistrationId(oauthToken.getAuthorizedClientRegistrationId())
                    .principal(authentication)
                    .build();
 
            // The Manager checks if the token exists and if it hasn't expired (it automatically refreshes it if needed)
            OAuth2AuthorizedClient authorizedClient = authorizedClientManager.authorize(authorizeRequest);
 
            // If a valid token was successfully retrieved, inject it into the Authorization Header
            if (authorizedClient != null && authorizedClient.getAccessToken() != null) {
                request.getHeaders().setBearerAuth(authorizedClient.getAccessToken().getTokenValue());
            }
        }
 
        // Proceed with the original HTTP request execution
        return execution.execute(request, body);
    }
}
``` 

Now our BFF is ready to talk with the API. Open the bff controller and make these new changes:

```java
// imports...
@RestController
public class BffController {

  private final RestClient restClient;

  public BffController(RestClient restClient) {
      this.restClient = restClient;
  }

  @GetMapping("/api")
  public String testeBackend() {

      return restClient.get()
              .uri("http://localhost:8082/avatar")
              .retrieve()
              .body(String.class);
  }
    // other methods...

}
```

### Testing

Run both BFF/API applications, and follow the next steps:

1. Open our new controller on your browser "localhost:8081/api". (If you dont have an open session, Keycloak login page should appear, make login and it will redirect you back to the /api)

2. The BFF will send our request with the `Access Token`, and if its a valid one, the API returns you the content you wanted to access. In our test, the scenario is /api -> GET -> /avatar.

![Image03](../images/protectingApi_api.png)

> [!Important] Result
> If you see this content, congratulations! Your BFF is connected to your protected API with authentication filter.
---
### Next tutorial

You finished the "Protecting Your API with Authentication tutorial", we strongly recommend you to do this next tutorial: [Protecting Your Api with Authorization]()







