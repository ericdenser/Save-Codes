<script setup lang="ts">
import { useAuthStore } from '@/stores/auth'

// Puxa as infos do Pinia 
const authStore = useAuthStore()

// Função para dar cores diferentes dependendo da Role
const getRoleCor = (role: string) => {
  if (role.includes('ADMIN')) return 'badge-admin'
  if (role.includes('USER')) return 'badge-user'
  return 'badge-padrao'
}
</script>

<template>
  <div class="perfil-container">
    <header class="cabecalho">
      <h2>Meu Perfil</h2>
      <router-link to="/tarefas" class="btn-voltar">Voltar para Tarefas</router-link>
    </header>

    <main class="perfil-card">
      <div class="avatar-area">
        <div class="avatar-circulo">
          {{ authStore.user?.nome?.charAt(0).toUpperCase() }}
        </div>
        <h3 class="nome-usuario">{{ authStore.user?.nome }}</h3>
        <p class="username-texto">@{{ authStore.user?.username }}</p>
      </div>

      <div class="divisor"></div>

      <div class="info-area">
        <h4>Permissões de Acesso (Roles)</h4>
        
        <div v-if="!authStore.user?.roles || authStore.user?.roles.length === 0" class="msg-vazia">
          Nenhuma permissão especial vinculada a esta conta.
        </div>
        
        <div v-else class="tags-container">
          <span 
            v-for="role in authStore.user?.roles" 
            :key="role" 
            class="badge-role"
            :class="getRoleCor(role)"
          >
            {{ role.replace('ROLE_', '') }}
          </span>
        </div>
      </div>
    </main>
  </div>
</template>

<style scoped>
.perfil-container {
  max-width: 500px;
  margin: 40px auto;
  font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
  padding: 20px;
}

.cabecalho {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 30px;
  padding-bottom: 10px;
  border-bottom: 2px solid #eee;
}

h2 { color: #2c3e50; margin: 0; }

.btn-voltar {
  background-color: #95a5a6;
  color: white;
  text-decoration: none;
  padding: 8px 16px;
  border-radius: 4px;
  font-weight: bold;
  font-size: 14px;
  transition: 0.2s;
}

.btn-voltar:hover { background-color: #7f8c8d; }

.perfil-card {
  background: white;
  border-radius: 12px;
  box-shadow: 0 4px 15px rgba(0,0,0,0.08);
  padding: 30px;
  text-align: center;
}

.avatar-area {
  margin-bottom: 20px;
}

.avatar-circulo {
  width: 80px;
  height: 80px;
  background-color: #42b983; /* Cor principal do Vue */
  color: white;
  font-size: 36px;
  font-weight: bold;
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  margin: 0 auto 15px auto;
  box-shadow: 0 4px 10px rgba(66, 185, 131, 0.3);
}

.nome-usuario {
  margin: 0;
  font-size: 24px;
  color: #2c3e50;
}

.username-texto {
  margin: 5px 0 0 0;
  color: #7f8c8d;
  font-size: 14px;
}

.divisor {
  height: 1px;
  background-color: #ecf0f1;
  margin: 25px 0;
}

.info-area h4 {
  color: #34495e;
  margin-top: 0;
  margin-bottom: 15px;
  font-size: 16px;
}

.tags-container {
  display: flex;
  flex-wrap: wrap;
  justify-content: center;
  gap: 10px;
}

.badge-role {
  padding: 6px 14px;
  border-radius: 20px;
  font-size: 12px;
  font-weight: 800;
  letter-spacing: 0.5px;
  text-transform: uppercase;
  color: white;
}

/* Cores dinâmicas para as Roles */
.badge-admin { background-color: #e74c3c; } /* Vermelho para Admin */
.badge-user { background-color: #3498db; }  /* Azul para User */
.badge-padrao { background-color: #9b59b6; } /* Roxo para outros */

.msg-vazia {
  color: #7f8c8d;
  font-style: italic;
  font-size: 14px;
}
</style>