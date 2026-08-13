<template>
  <div class="pet-container" :class="currentMood" @click="handleClick">
    <div class="speech-bubble">爱上雷神~</div>
    <div class="pet-body">
      <div class="pet-hand left"></div>
      <div class="pet-hand right"></div>
    </div>
    <PetAvatar :mood="currentMood" :size="180" />
  </div>
</template>

<script setup>
import { ref, computed } from 'vue'
import PetAvatar from './PetAvatar.vue'

const emit = defineEmits(['mood-change'])

const currentMood = ref('neutral')

const moods = {
  happy: { text: '😃 开心鼓掌!', color: '#10b981' },
  sleep: { text: '😴 犯困打盹...', color: '#64748b' },
  excited: { text: '🤪 高兴呐喊!', color: '#ec4899' },
  neutral: { text: '😊 安静待机中', color: '#fbbf24' }
}

function setMood(mood) {
  currentMood.value = mood
  emit('mood-change', moods[mood])
}

function resetMood() {
  setMood('neutral')
}

function handleClick() {
  const randomMoods = ['happy', 'excited']
  const random = randomMoods[Math.floor(Math.random() * randomMoods.length)]
  setMood(random)
  setTimeout(() => resetMood(), 3000)
}

defineExpose({ setMood, resetMood })
</script>

<style scoped>
.pet-container {
  position: relative;
  width: 200px;
  height: 280px;
  margin: 0 auto 1rem;
  cursor: pointer;
}
.pet-body {
  position: absolute;
  bottom: 0;
  left: 50%;
  transform: translateX(-50%);
  width: 140px;
  height: 100px;
  background: linear-gradient(135deg, #fbbf24, #8b5cf6);
  border-radius: 70px 70px 50px 50px;
  transition: all 0.3s ease;
}
.pet-container svg {
  position: absolute;
  bottom: 70px;
  left: 50%;
  transform: translateX(-50%);
  transition: all 0.3s ease;
}
.pet-hand {
  position: absolute;
  width: 35px;
  height: 60px;
  background: linear-gradient(135deg, #8b5cf6, #6366f1);
  border-radius: 17px;
  bottom: 30px;
  transition: transform 0.3s ease;
}
.pet-hand.left { left: -10px; transform-origin: top center; }
.pet-hand.right { right: -10px; transform-origin: top center; }

.speech-bubble {
  position: absolute;
  top: -70px;
  right: 10px;
  background: white;
  padding: 0.75rem 1rem;
  border-radius: 16px;
  box-shadow: 0 10px 25px -5px rgba(0, 0, 0, 0.1);
  font-size: 1rem;
  font-weight: 600;
  color: #0f172a;
  opacity: 0;
  transform: scale(0);
  transform-origin: bottom left;
  transition: all 0.3s ease;
}
.speech-bubble::after {
  content: '';
  position: absolute;
  bottom: -8px;
  left: 20px;
  border-width: 8px 8px 0;
  border-style: solid;
  border-color: white transparent transparent transparent;
}

@keyframes bounce {
  0%, 100% { transform: translateX(-50%) translateY(0); }
  50% { transform: translateX(-50%) translateY(-10px); }
}
@keyframes clap {
  0%, 100% { transform: rotate(-20deg); }
  50% { transform: rotate(20deg); }
}
.pet-container.happy .pet-hand.left { animation: clap 0.5s infinite alternate; }
.pet-container.happy .pet-hand.right { animation: clap 0.5s infinite alternate-reverse; }
.pet-container.happy .pet-body { animation: bounce 0.5s infinite; }
.pet-container.happy svg { animation: bounce 0.5s infinite; }

@keyframes jump {
  0%, 100% { transform: translateX(-50%) translateY(0); }
  25% { transform: translateX(-50%) translateY(-15px) rotate(-5deg); }
  75% { transform: translateX(-50%) translateY(-15px) rotate(5deg); }
}
@keyframes wave-arms {
  0%, 100% { transform: rotate(-40deg); }
  50% { transform: rotate(10deg); }
}
.pet-container.excited .pet-hand.left { animation: wave-arms 0.3s infinite alternate; }
.pet-container.excited .pet-hand.right { animation: wave-arms 0.3s infinite alternate-reverse; }
.pet-container.excited .pet-body { animation: jump 0.6s infinite; }
.pet-container.excited svg { animation: jump 0.6s infinite; }
.pet-container.excited .speech-bubble { opacity: 1; transform: scale(1); }

.pet-container.sleep svg {
  animation: sleep-nod 3s infinite ease-in-out;
}
@keyframes sleep-nod {
  0%, 100% { transform: translateX(-50%) rotate(0deg); }
  25% { transform: translateX(-50%) rotate(-5deg); }
  75% { transform: translateX(-50%) rotate(5deg); }
}
</style>