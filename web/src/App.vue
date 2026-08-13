<template>
  <div class="app-container">
    <Transition name="fade" mode="out-in">
      <article class="page" v-if="currentView === 'home'">
        <HeroSection />
        <ProjectOverview @navigate="navigate" />
        <FeatureGrid />
        <WidgetPreview />
        <CustomizationSection />
        <TechArchitecture />
        <InspirationSection />
        <FooterSection />
        <FloatingPet />
      </article>
      <WidgetDemo v-else-if="currentView === 'demo'" @back="navigate('home')" />
    </Transition>
  </div>
</template>

<script setup>
import { ref } from 'vue'
import HeroSection from './components/HeroSection.vue'
import ProjectOverview from './components/ProjectOverview.vue'
import FeatureGrid from './components/FeatureGrid.vue'
import WidgetPreview from './components/WidgetPreview.vue'
import CustomizationSection from './components/CustomizationSection.vue'
import TechArchitecture from './components/TechArchitecture.vue'
import InspirationSection from './components/InspirationSection.vue'
import FooterSection from './components/FooterSection.vue'
import FloatingPet from './components/FloatingPet.vue'
import WidgetDemo from './components/WidgetDemo.vue'

const currentView = ref('home')

function navigate(view) {
  currentView.value = view
}
</script>

<style>
.fade-enter-active,
.fade-leave-active {
  transition: opacity 0.3s ease;
}

.fade-enter-from,
.fade-leave-to {
  opacity: 0;
}
</style>