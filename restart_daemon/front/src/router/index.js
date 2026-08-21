import { createRouter, createWebHistory } from 'vue-router'
import TelemetryView from '../views/TelemetryView.vue'
import BlackBoxSettingsView from '../views/BlackBoxSettingsView.vue'

const routes = [
  {
    path: '/',
    name: 'telemetry',
    component: TelemetryView
  },
  {
    path: '/blackbox',
    name: 'blackbox',
    component: BlackBoxSettingsView
  }
]

const router = createRouter({
  history: createWebHistory(process.env.BASE_URL),
  routes
})

export default router