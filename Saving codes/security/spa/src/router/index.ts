import { createRouter, createWebHistory } from 'vue-router'
import LoginView from '../views/LoginView.vue'
import { useAuthStore } from '@/stores/auth'
import api from '@/services/api'

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes: [
    { path: '/', name: 'login', component: LoginView },
    { path: '/tarefas', name: 'tarefas', component: () => import('../views/HomeView.vue') },
    { path: '/perfil', name: 'perfil', component: () => import('../views/UserInfoView.vue') },
    { path: '/admin', name: 'painel-admin', component: () => import('../views/AdminView.vue'), meta: { requiresRole: ['ROLE_ADMIN']}},
  ]
})

// O GUARDA DE SEGURANÇA
router.beforeEach(async (to, from, next) => {
  const authStore = useAuthStore()
  await authStore.checkAuth()

  // Não tá logado e tentou ir pra área logada? Login.
  if (to.name !== 'login' && !authStore.isAuthenticated) {
    return next({ name: 'login' })
  } 
  // Tá logado e tentou ir pra tela de Login? Vai pra Home.
  if (to.name === 'login' && authStore.isAuthenticated) {
    return next({ name: 'tarefas' })
  } 
  if (to.meta.requiresRole) {
    const requiredRoles = to.meta.requiresRole as string[];
    try {
      const rolesParam = requiredRoles.join(',');
      const response = await api.get(`/check-role?role=${rolesParam}`)
      const hasRole = response.data.hasRole

      if (!hasRole) {
        alert('Acesso Negado: You dont have the required role to access this resource.');
        return next({ name: 'tarefas' })
      }

    } catch (e) {
      // Se a requisição falhar
      console.error("Erro ao validar role no BFF:", e);
      
      // bloqueia o acesso aqui também
      return next({ name: 'tarefas' });
    }
  }

  next()

})

export default router
