<script setup lang="ts">
import { ref, onMounted } from 'vue'
import api from '@/services/api' 

const tarefas = ref<any[]>([]) 
const erro = ref<string>('')

const carregarTarefas = async () => {
  try {
    const resposta = await api.get('/buscar-tarefas')
    tarefas.value = resposta.data
  } catch (e) {
    erro.value = 'Acesso Negado ou Sessão Expirada. Por favor, faça login novamente.'
    console.error(e)
  }
}

const fazerLogout = () => {
  // Pega o valor do cookie de segurança (XSRF-TOKEN) que o Spring enviou
  const match = document.cookie.match(new RegExp('(^| )XSRF-TOKEN=([^;]+)'))
  const csrfToken = match ? match[2] : ''

  // Cria um formulário invisível no HTML
  const form = document.createElement('form')
  form.method = 'POST'
  form.action = '/api/logout' // Usa o proxy do Vite

  // Coloca a chave CSRF dentro do formulário
  if (csrfToken) {
    const inputCsrf = document.createElement('input')
    inputCsrf.type = 'hidden'
    inputCsrf.name = '_csrf' // O Spring exige esse nome exato
    inputCsrf.value = csrfToken
    form.appendChild(inputCsrf)
  }

  // EQUIVALENTE:
  //<form method="POST" action="/api/logout">
   // <input type="hidden" name="_csrf" value="abc123">
  //</form>

  // Anexa na tela e aperta o "Enter" invisível
  document.body.appendChild(form)
  form.submit() 
}

onMounted(() => {
  carregarTarefas()
})
</script>

<template>
  <div class="home-container">
    <header class="cabecalho">
      <h2>Minhas Tarefas</h2>
      
      <div class="acoes-cabecalho">
        <router-link to="/perfil" class="btn-perfil">Meu Perfil</router-link>
        <router-link to="/admin">Painel Admin</router-link>
        <button @click="fazerLogout" class="btn-sair">Sair do Sistema</button>
      </div>
    </header>

    <main>
      <div v-if="erro" class="msg-erro">
        {{ erro }}
        <br><br>
        <router-link to="/">Voltar para o Login</router-link>
      </div>

      <div v-else-if="tarefas.length === 0" class="msg-vazia">
        Carregando tarefas ou nenhuma tarefa encontrada...
      </div>

      <ul v-else class="lista-tarefas">
        <li v-for="tarefa in tarefas" :key="tarefa.id" class="tarefa-item">
          <strong>{{ tarefa.titulo }}</strong>
          <span class="badge-id">ID: {{ tarefa.id }}</span>
        </li>
      </ul>
    </main>
  </div>
</template>

<style scoped>

.home-container {
  max-width: 600px;
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


.acoes-cabecalho {
  display: flex;
  gap: 10px;
  align-items: center;
}

.btn-perfil {
  background-color: #3498db;
  color: white;
  text-decoration: none;
  padding: 8px 16px;
  border-radius: 4px;
  font-weight: bold;
  font-size: 14px;
  transition: 0.2s;
}

.btn-perfil:hover {
  background-color: #2980b9;
}

.btn-sair {
  background-color: #e74c3c;
  color: white;
  border: none;
  padding: 8px 16px;
  border-radius: 4px;
  cursor: pointer;
  font-weight: bold;
  font-size: 14px;
  transition: 0.2s;
}

.btn-sair:hover { background-color: #c0392b; }

.lista-tarefas { list-style: none; padding: 0; }
.tarefa-item {
  background: white;
  margin-bottom: 12px;
  padding: 16px;
  border-radius: 8px;
  border-left: 5px solid #42b983; 
  box-shadow: 0 2px 8px rgba(0,0,0,0.05);
  display: flex;
  justify-content: space-between;
  align-items: center;
}
.badge-id {
  background: #ecf0f1;
  color: #7f8c8d;
  padding: 4px 8px;
  border-radius: 12px;
  font-size: 12px;
  font-weight: bold;
}
.msg-erro {
  background: #fadbd8; color: #c0392b; padding: 15px; border-radius: 8px; text-align: center; font-weight: bold;
}
.msg-vazia { text-align: center; color: #7f8c8d; font-style: italic; margin-top: 40px; }
</style>