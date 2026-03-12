import axios from 'axios'
import { useAuthStore } from '@/stores/auth'

const api = axios.create({
  baseURL: '/api',
  withCredentials: true,

  // Eu sou um código Javascript, por favor, não me redirecione para telas de login, apenas me dê um Erro 401
  headers: {
    'X-Requested-With': 'XMLHttpRequest'
  }
})


api.interceptors.response.use(
  (response) => {
    return response // Se deu 200 OK, deixa passar normal
  },
  (error) => {
    // Garante que o servidor respondeu algo antes de checar o status
    if (error.response) {
      
      // 401: Sessão expirou ou não existe no Redis
      if (error.response.status === 401) {
        const authStore = useAuthStore()
        authStore.clearAuth()
        
        // Chuta pro login com Hard Reset (Limpa cache do Pinia de todas as abas)
        window.location.href = '/' 
      } 
      
      // 403: Está logado, mas não tem permissão 
      else if (error.response.status === 403) {
        alert('Acesso Negado: Você não tem permissão para realizar esta ação.')
        // Aqui sim, se você quisesse redirecionar para uma tela de erro sem limpar a memória,
        // você poderia usar um router.push('/acesso-negado')
      }
    }

    //SEMPRE rejeita a promessa no final para quebrar o fluxo lá no componente Vue
    return Promise.reject(error)
  }
)

export default api
