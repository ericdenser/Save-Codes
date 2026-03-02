> [Learning](../README.md)>[Learning Paths](./README.md)>Protecting the API

# Protecting the API


# Prerequisites

1. Java and Maven configured in your development environment.

    - [Install and configure Java and Maven in a Windows box.](java/Preparing%20a%20Java%20development%20environment%20on%20Windows.md?ref_type=heads)
    - [Install and configure Java and Maven in a Debian Linux box.](java/SettingUpDebianBoxForJavaDevelopment.md?ref_type=heads)
    - [Install and configure Java and Maven in a workstation in MackLEAPS facilities.](java/SettingUpJavaDevelopmentEnvironmentInMackleapsWorkstations.md)

2. Visual Studio Code installed in your development enviromnment with Java and Spring Boot extensions configured.

    - [Install Visual Studio Code in a Windows box.](ide/InstallVSCodeInWindows.md)
    - [Install Visual Studio Code in a Debian Linux box.](ide/InstallVSCode-Debian.md)
  
    
3. [Install Visual Studio Code extensions for Java development.](ide/VSCodeExtensionsForJava.md)

4. [Docker installed](docker/DockerOnUbuntu.md)

5. [Knowledge about Spring Boot](LearningPaths/MyFirstJavaRestAPI.md)



## BFF (Backend For Frontend)

The BFF will serve as an intermediary between the backend and the frontend. This way, we can implement Keycloak within the BFF so that the frontend does not have access to Access Tokens. To achieve this, we will use [Spring Initializr](https://start.spring.io/) to create the Spring Boot application for our BFF.



1.  Parameters to create the project:

- Project: Maven
- Language: Java
- Spring Boot: 4.0.2
- Project Metadata:
- Group br.mackenzie.mackleaps
- Artifact: BFF
- Name: BFF
- Packaging: Jar
- Configuration: YAML

2.  Select the following dependecies:

- Spring Web
- OAuth2 Client
- Spring Security
- Spring Data Redis (Access+Driver)

For more details about Spring project initialization, refer to: [Create a Spring Boot Java Project](java/CreateASpringBootJavaProject.md).

After generating the project, a ZIP file will be downloaded to your machine. Unzip it and open the project in VS Code.

Make sure you have the required VS Code extensions for Java backend development by following this guide: [Install Visual Studio Code Extensions for Java Development](ide/VSCodeExtensionsForJava.md).

3.  Before proceeding, it will be necessary to add some dependencies manually - Open the pom.xml file and add the following dependencies:* 


```xml
    <dependency>
        <groupId>org.springframework.boot</groupId>
        <artifactId>spring-boot-starter-web</artifactId>
    </dependency>

    <dependency>
        <groupId>org.springframework.boot</groupId>
        <artifactId>spring-boot-starter-oauth2-client</artifactId>
    </dependency>

    <dependency>
        <groupId>org.springframework.session</groupId>
        <artifactId>spring-session-data-redis</artifactId>
    </dependency>

```

4.  Make sure the pom.xml file looks like this:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<project xmlns="http://maven.apache.org/POM/4.0.0" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
	xsi:schemaLocation="http://maven.apache.org/POM/4.0.0 https://maven.apache.org/xsd/maven-4.0.0.xsd">
	<modelVersion>4.0.0</modelVersion>
	<parent>
		<groupId>org.springframework.boot</groupId>
		<artifactId>spring-boot-starter-parent</artifactId>
		<version>4.0.2</version>
		<relativePath/> <!-- lookup parent from repository -->
	</parent>
	<groupId>br.mackenzie.mackeleaps</groupId>
	<artifactId>BFF</artifactId>
	<version>0.0.1-SNAPSHOT</version>
	<name>BFF</name>
	<description>Demo project for Spring Boot</description>
	<url/>
	<licenses>
		<license/>
	</licenses>
	<developers>
		<developer/>
	</developers>
	<scm>
		<connection/>
		<developerConnection/>
		<tag/>
		<url/>
	</scm>
	<properties>
		<java.version>21</java.version>
	</properties>
	<dependencies>
		<dependency>
			<groupId>org.springframework.boot</groupId>
			<artifactId>spring-boot-starter-data-redis</artifactId>
		</dependency>
		<dependency>
			<groupId>org.springframework.boot</groupId>
			<artifactId>spring-boot-starter-security</artifactId>
		</dependency>
		<dependency>
			<groupId>org.springframework.boot</groupId>
			<artifactId>spring-boot-starter-security-oauth2-client</artifactId>
		</dependency>
		<dependency>
			<groupId>org.springframework.boot</groupId>
			<artifactId>spring-boot-starter-webmvc</artifactId>
		</dependency>
		<dependency>
			<groupId>org.springframework.boot</groupId>
			<artifactId>spring-boot-starter-data-redis-test</artifactId>
			<scope>test</scope>
		</dependency>
		<dependency>
			<groupId>org.springframework.boot</groupId>
			<artifactId>spring-boot-starter-security-oauth2-client-test</artifactId>
			<scope>test</scope>
		</dependency>
		<dependency>
			<groupId>org.springframework.boot</groupId>
			<artifactId>spring-boot-starter-security-test</artifactId>
			<scope>test</scope>
		</dependency>
		<dependency>
			<groupId>org.springframework.boot</groupId>
			<artifactId>spring-boot-starter-webmvc-test</artifactId>
			<scope>test</scope>
		</dependency>
		<dependency>
			<groupId>org.springframework.boot</groupId>
			<artifactId>spring-boot-starter-web</artifactId>
		</dependency>
		<dependency>
			<groupId>org.springframework.boot</groupId>
			<artifactId>spring-boot-starter-oauth2-client</artifactId>
		</dependency>
		<dependency>
			<groupId>org.springframework.session</groupId>
			<artifactId>spring-session-data-redis</artifactId>
		</dependency>

	</dependencies>

	<build>
		<plugins>
			<plugin>
				<groupId>org.springframework.boot</groupId>
				<artifactId>spring-boot-maven-plugin</artifactId>
			</plugin>
		</plugins>
	</build>

</project>
```



## Docker Containers

The BFF will run in terminal. The Keycloak, Postgres and Redis, will run in Docker containers. To set this up, create a file in the root of your project called `docker-compose.yml`. This file will contain the configurations for the container that will run Keycloak (authentication system), Redis (database that will store access tokens), and Postgres (database that will handle data persistence for Keycloak).

> [!note]
> Since we are using Docker to run Keycloak, it is important to configure data persistence for Keycloak — that is, use a database (in this case, PostgreSQL) to store Keycloak’s configuration and users. This ensures that if the container is stopped, Keycloak’s settings will not be lost.

To achieve this, it is necessary to configure a file called `docker-compose.yml`, which contains the Docker settings, including an element called volumes. This is where Keycloak’s configuration data will be stored.

5.  Create a folder in the project root called "volumes". Inside this folder, create two additional folders named `"postgres-data"` and `"keycloak-data"`.

6.  Create a file in the project root called `"docker-compose.yml"` with the following content: 

```yaml
services:
  postgres:
    image: postgres:15
    environment:
      POSTGRES_DB: keycloak
      POSTGRES_USER: keycloak
      POSTGRES_PASSWORD: password
    ports:
      - "5432:5432"
    volumes:
      - ./volumes/postgres-data:/var/lib/postgresql/data
 
 
  redis:
    image: redis:7-alpine
    ports:
      - "6379:6379"
 
  keycloak:
    image: quay.io/keycloak/keycloak:26.1.2
    command: start-dev
    environment:
      KC_DB: postgres
      KC_DB_URL: jdbc:postgresql://postgres:5432/keycloak
      KC_DB_USERNAME: keycloak
      KC_DB_PASSWORD: password
      KEYCLOAK_ADMIN: admin
      KEYCLOAK_ADMIN_PASSWORD: admin
    ports:
      - "8080:8080"
    volumes:
      - ./volumes/keycloak-data:/opt/keycloak/themes
    depends_on:
      - postgres
```

7.  After creating these folders, the project structure will be similar to:

```bash
BFF/
├── .mvn/
├── src/
├── target/
├── volumes/
│   ├── keycloak-data/
│   └── postgres-data/
├── .gitattributes
├── .gitignore
├── docker-compose.yml
├── HELP.md
├── mvnw
├── mvnw.cmd
└── pom.xml

```

> [!note] 
> It may take a while to start the Keycloak page on localhost.
 
## Keycloak

Now let’s configure Keycloak, creating a Realm (space that will manage a group of Clients and Users), a Client (represents applications and services that can access Keycloak to authenticate and authorize User) and a User.

To access Keycloak it is necessary to run the Docker containers that we just configured.

1.  In the terminal enter the project folder

2.  Run the command: 
``` bash 
docker compose up -d
```
3.  To verify that all containers started, run the command: 
```bash
docker ps
```

To configure Keycloak:
4.  In your browser access the route http://localhost:8080

5.  Enter the credentials: Username: admin / Password: admin

6.  The page http://localhost:8080 may take a few minutes to load.

Configurations

7.  In the upper left corner there will be a tab with the Master realm. Click on the tab, the button "Create realm" will appear. Click it.
![](LearningPaths/images/1.png)

8.  Create the realm with the name **BFF**
![](LearningPaths/images/2.png)

Creating a Client

9.  On the left side there are some options, select **Clients**

10. Click the button **Create Client**
![](LearningPaths/images/3.png)

11. In Client ID and Name enter: **spring-bff**
![](LearningPaths/images/4.png)

12. Select Client authentication
![](LearningPaths/images/5.png)

13. Set the URLs according to the image.
![](LearningPaths/images/6.png)

14. Creating User, on the left side select the User option
![](LearningPaths/images/7.png)

15. Define the fields as in the image below
![](LearningPaths/images/8.png)

16. Go to credentials and set the password: **admin**
![](LearningPaths/images/9.png)

17. Disable the temporary option and click **Save**.
![](LearningPaths/images/10.png)

## Application.yaml

Now that Keycloak is configured, let's create an `application.yaml` file, which is responsible for configuring the application's properties.
One of the properties is ***client-secret***, however this property is individual.

1.  Open Keycloak, select the Clients tab, and enter the client we created.

![](LearningPaths/images/x.png)

2.  Select the **Credentials** option, copy the code, and paste it into your `application.yaml`.

![](LearningPaths/images/y.png)

3.  Make sure your `application.yaml` file is formatted as follows:

```yaml
spring:
  application:
    name: bff-project
 
  data:
    redis:
      host: localhost
      port: 6379
 
  security:
    oauth2:
      client:
        registration:
          keycloak:
            client-id: spring-bff
            client-secret: COPY HERE !!!!
            scope: openid, profile, email
            authorization-grant-type: authorization_code
            redirect-uri: "{baseUrl}/login/oauth2/code/{registrationId}"
        provider:
          keycloak:
            issuer-uri: http://localhost:8080/realms/BFF
  session:
    store_type: redis
 
server:
  port: 8081
 
logging:
  level:
    org:
      springframework:
        security: DEBUG
        oauth2: DEBUG
        web:
          client:
            RestTemplate: DEBUG
```

## BffController

Now let's create a Java class called BffController; this class will handle the requests and redirects.

1.  Create the class BffController with the content below.

```java
package br.mackenzie.mackleaps.BFF.controller;

import org.springframework.security.core.annotation.AuthenticationPrincipal;
import org.springframework.security.oauth2.core.oidc.user.OidcUser;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RestController;

@RestController

public class BffController {

    @GetMapping("/protected")
    public String protectedPag(@AuthenticationPrincipal OidcUser usuario){

        return "Bem-vindo a area segura, " + usuario.getFullName() +
        "! Seu email é: " + usuario.getEmail() +
        "Seu TOKEN é: " + usuario.getIdToken().getTokenValue();
    }

    @GetMapping("/public")
    public String publicPage() {
        return "This page is public!";
    }
}

```

### About this class  
* **@RestController**: It indicates that the class is not just a regular class, but a web Resource Controller.

* **@AutheticationPrincipal**: The `@AuthenticationPrincipal` annotation acts as a smart shortcut that extracts the authenticated user directly from the Spring security context and injects it as a parameter into your method, eliminating the need to manually retrieve session or token data; it identifies who is making the request and delivers the user object (such as OidcUser) ready for use, allowing you to access name, email, and permissions.

* **OidcUser**: This indicates that the OpendId Connect protocol is being used; the OidcUser contains the user claims (information) returned by the identity provider (Keycloak).


## SecurityConfig

Now let's create another class that will configure Spring Security. Spring Security already has a default configuration, but by creating a `config` class, we will dictate some rules for how Spring should behave.

1.  Create the `SecurityConfig` class with the following content:

```java
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.security.config.Customizer;
import org.springframework.security.config.annotation.web.builders.HttpSecurity;
import org.springframework.security.config.annotation.web.configuration.EnableWebSecurity;
import org.springframework.security.oauth2.client.oidc.web.logout.OidcClientInitiatedLogoutSuccessHandler;
import org.springframework.security.oauth2.client.registration.ClientRegistrationRepository;
import org.springframework.security.web.SecurityFilterChain;

@Configuration
@EnableWebSecurity
public class SecurityConfig {

    private final ClientRegistrationRepository clientRegistrationRepository;

    SecurityConfig(ClientRegistrationRepository clientRegistrationRepository){
        this.clientRegistrationRepository = clientRegistrationRepository;
    }

	//  By default, Spring Security use an in-memory storage mechanism. This configuration overrides that default to use the HTTP session instead and persist 	in redis
	@Bean
    public OAuth2AuthorizedClientRepository authorizedClientRepository() {
        return new HttpSessionOAuth2AuthorizedClientRepository();
    }

    @Bean
    public SecurityFilterChain filterChain(HttpSecurity http) throws Exception {
        http
            // Disabling CSRF temporarily for initial testing purposes
            .csrf(csrf -> csrf.disable())
            
            // Configuring endpoint access rules
            .authorizeHttpRequests(auth -> auth
                .requestMatchers("/public").permitAll() 
                .anyRequest().authenticated()      
            )

            // Enabling OAuth2 Login (Redirects to Keycloak when unauthenticated)
            .oauth2Login(Customizer.withDefaults()); 
		
        return http.build();
    }
}
```


### About this class

SecurityFilterChain: This is the core of Spring Security. It acts as a sequence of "filters" (like security guards) that every incoming HTTP request must pass through before reaching your Controller. If a request fails a filter's rule, it is blocked or redirected.

@Configuration: Tells Spring that this class acts as an instruction manual. During startup, Spring reads this class to set up internal configurations.

@EnableWebSecurity: Activates Spring's security system. It tells Spring to apply the SecurityFilterChain to all incoming HTTP requests.

@Bean: Turns the method's return object into a component managed by the Spring framework.


[!WARNING] About CSRF
In this initial configuration, we explicitly disabled CSRF (.csrf(csrf -> csrf.disable())). Since our BFF uses Cookies to maintain the user's session, disabling CSRF leaves the application vulnerable to Cross-Site Request Forgery attacks. We are doing this temporarily to make our first API tests easier. In a future tutorial about Frontend integration, we will enable CSRF and configure the necessary protections.


*  You can learn more about the filter chain on the [Spring Documentation](https://docs.spring.io/spring-security/reference/servlet/architecture.html)



## BffApplication

Another class that is needed to configure is the BffApplication, it serves as a kind of 'main' class.

1.  In the `BffApplication.java`, add the annotation "@EnableRedisHttpSession", it will create and set up a filter to look for active sessions and work with them on the security context from values stored in Redis.

```java
package br.mackenzie.mackleaps.BFF;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.session.data.redis.config.annotation.web.http.EnableRedisHttpSession;

@EnableRedisHttpSession
@SpringBootApplication
public class BffApplication {

	public static void main(String[] args) {
		SpringApplication.run(BffApplication.class, args);
	}

}

```

## Running the Project

To run the project:

1.  Open the terminal and navigate to the project folder.
2.  To run Maven, execute the command: 

```bash
mvn spring-boot:run
```
3.  Access the route `http://localhost:8081/` to enter the unprotected route.
4.  Access the route `http://localhost:8081/protected` to enter a page with Keycloak validation.

---

## Implementing the OIDC Logout

Now that you successfully authenticated and accessed the protected route, you might notice that there is no way to log out. Closing the browser tab won't kill your Keycloak session entirely. 

To properly log out in a Single Sign-On (SSO) architecture, we must perform a **RP-Initiated Logout**. This means our BFF must not only destroy the local Spring session (Redis) but also redirect the user to Keycloak to destroy the global SSO session.

### 1. Configuring Keycloak Logout URL
Before touching the code, we need to authorize our frontend to be redirected back after Keycloak finishes the logout process.

1. Open your Keycloak admin console (`http://localhost:8080`).
2. Go to **Clients** -> select `spring-bff`.
3. Scroll down to the **Logout settings** section.
4. In the **Valid post logout redirect URIs** field, add `http://localhost:8081/public`.
5. Click **Save**.

### 2. Updating SecurityConfig

Now, let's update our `SecurityConfig` class to handle the logout flow. We will add a `ClientRegistrationRepository` to build the correct Keycloak URLs and configure the `.logout()` chain.

Update your `SecurityConfig.java` to match this:

```java
package br.mackenzie.mackleaps.BFF.config;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.security.config.Customizer;
import org.springframework.security.config.annotation.web.builders.HttpSecurity;
import org.springframework.security.config.annotation.web.configuration.EnableWebSecurity;
import org.springframework.security.oauth2.client.oidc.web.logout.OidcClientInitiatedLogoutSuccessHandler;
import org.springframework.security.oauth2.client.registration.ClientRegistrationRepository;
import org.springframework.security.web.SecurityFilterChain;

@Configuration
@EnableWebSecurity
public class SecurityConfig {

    // Injecting the repository that holds our Keycloak settings from application.yaml
    private final ClientRegistrationRepository clientRegistrationRepository;

    public SecurityConfig(ClientRegistrationRepository clientRegistrationRepository){
        this.clientRegistrationRepository = clientRegistrationRepository;
    }

    @Bean
    public SecurityFilterChain filterChain(HttpSecurity http) throws Exception {
        http
            .csrf(csrf -> csrf.disable())
            .authorizeHttpRequests(auth -> auth
                .requestMatchers("/public").permitAll() 
                .anyRequest().authenticated()       
            )
            .oauth2Login(Customizer.withDefaults()) 
            
            // NEW LOGOUT CONFIGURATION
            .logout(logout -> logout
                .logoutUrl("/logout") // The endpoint that triggers the logout process
                .invalidateHttpSession(true) // 1. Destroys the local Spring session (Redis)
                .clearAuthentication(true) // 2. Clears the security context
                .deleteCookies("SESSION") // 3. Commands the browser to delete the Session Cookie
                .logoutSuccessHandler(oidcLogoutSuccessHandler()) // 4. Redirects to Keycloak
            ); 

        return http.build();
    }

    // Custom method to handle the OIDC Logout Redirect
    private OidcClientInitiatedLogoutSuccessHandler oidcLogoutSuccessHandler(){
        OidcClientInitiatedLogoutSuccessHandler successHandler = 
            new OidcClientInitiatedLogoutSuccessHandler(clientRegistrationRepository);
        
        // Tells Keycloak where to send the user AFTER destroying the SSO session
        successHandler.setPostLogoutRedirectUri("{baseUrl}/public");
 
        return successHandler;
    }
}
```
How the Logout Chain Works
When the user triggers the /logout endpoint, Spring Security executes these steps in order:

Local Destruction: invalidateHttpSession and clearAuthentication wipe out the user's data from the BFF's memory and our Redis database.

Cookie Deletion: deleteCookies("SESSION") ensures the browser throws away its local key.

The Global Redirect: The oidcLogoutSuccessHandler builds a specific URL containing the user's ID Token and redirects the browser to Keycloak. Keycloak receives this, terminates the SSO session (KEYCLOAK_IDENTITY cookie), and redirects the user back to the {baseUrl}/public route.

3. Testing the Logout
Restart your Spring Boot application.

Go to http://localhost:8081/protected and ensure you are logged in.

In your browser, make a request to http://localhost:8081/logout. (Note: Since we temporarily disabled CSRF, a simple GET request from the browser bar might trigger it, but in production, this must be a POST request).

You will briefly see the Keycloak screen flash (or load) as it destroys the session, and then you will be seamlessly redirected to the /public page!

