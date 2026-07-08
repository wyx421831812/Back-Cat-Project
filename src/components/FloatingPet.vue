<template>
  <div 
    class="floating-pet" 
    :class="{ 'show-menu': showMenu, dragging: isDragging, [currentMood]: true }"
    :style="floatingStyle"
    @mousedown="startDrag"
    @click="handleClick"
  >
    <div class="pet-toggle" @click.stop="toggleMenu">
      <span v-if="showMenu">▼</span>
      <span v-else>▲</span>
    </div>
    
    <div class="pet-menu" v-show="showMenu">
      <button @click.stop="setMood('happy')" class="menu-btn">😊 开心</button>
      <button @click.stop="setMood('excited')" class="menu-btn">🤪 兴奋</button>
      <button @click.stop="setMood('sleep')" class="menu-btn">😴 睡觉</button>
      <button @click.stop="setMood('neutral')" class="menu-btn">😌 平静</button>
      <div class="menu-divider"></div>
      <button @click.stop="toggleVisible" class="menu-btn">{{ visible ? '隐藏' : '显示' }}</button>
      <button @click.stop="resetPosition" class="menu-btn">复位</button>
    </div>

    <PetAvatar :mood="currentMood" :size="90" />

    <div class="speech-bubble" v-show="showSpeech">
      {{ speechText }}
    </div>
  </div>
</template>

<script setup>
import { ref, computed, onMounted, onUnmounted } from 'vue'
import PetAvatar from './PetAvatar.vue'

const position = ref({ x: 20, y: 20 })
const isDragging = ref(false)
const dragOffset = ref({ x: 0, y: 0 })
const showMenu = ref(false)
const visible = ref(true)
const currentMood = ref('neutral')
const showSpeech = ref(false)
const speechText = ref('')

const speeches = [
  '爱上雷神~', '今天也要加油哦！', '摸摸头~', '好开心呀！', '别太累了~', '我陪着你~', '你真棒！', '休息一下吧~'
]

const floatingStyle = computed(() => {
  if (!visible.value) {
    return {
      transform: `translate(${position.value.x}px, ${position.value.y}px)`,
      opacity: 0,
      pointerEvents: 'none'
    }
  }
  return {
    transform: `translate(${position.value.x}px, ${position.value.y}px)`
  }
})

function startDrag(e) {
  if (e.target.closest('.pet-menu') || e.target.closest('.pet-toggle')) return
  
  isDragging.value = true
  dragOffset.value = {
    x: e.clientX - position.value.x,
    y: e.clientY - position.value.y
  }
  
  document.addEventListener('mousemove', onDrag)
  document.addEventListener('mouseup', stopDrag)
}

function onDrag(e) {
  if (!isDragging.value) return
  
  const maxX = window.innerWidth - 100
  const maxY = window.innerHeight - 100
  
  position.value = {
    x: Math.max(0, Math.min(maxX, e.clientX - dragOffset.value.x)),
    y: Math.max(0, Math.min(maxY, e.clientY - dragOffset.value.y))
  }
  
  localStorage.setItem('backpet-position', JSON.stringify(position.value))
}

function stopDrag() {
  isDragging.value = false
  document.removeEventListener('mousemove', onDrag)
  document.removeEventListener('mouseup', stopDrag)
}

function toggleMenu() {
  showMenu.value = !showMenu.value
}

function setMood(mood) {
  currentMood.value = mood
  
  if (mood === 'happy' || mood === 'excited') {
    showRandomSpeech()
  }
  
  moodTimeouts.push(setTimeout(() => {
    if (mood !== 'sleep') {
      currentMood.value = 'neutral'
    }
  }, 3000))
}

function handleClick() {
  const randomMoods = ['happy', 'excited']
  const random = randomMoods[Math.floor(Math.random() * randomMoods.length)]
  setMood(random)
}

function showRandomSpeech() {
  speechText.value = speeches[Math.floor(Math.random() * speeches.length)]
  showSpeech.value = true
  moodTimeouts.push(setTimeout(() => {
    showSpeech.value = false
  }, 2000))
}

function toggleVisible() {
  visible.value = !visible.value
}

function resetPosition() {
  position.value = { x: 20, y: 20 }
  localStorage.setItem('backpet-position', JSON.stringify(position.value))
}

let speechInterval = null
let moodTimeouts = []

onMounted(() => {
  const saved = localStorage.getItem('backpet-position')
  if (saved) {
    try {
      position.value = JSON.parse(saved)
    } catch (e) {
      console.error('Failed to parse saved position')
    }
  }
  
  speechInterval = setInterval(() => {
    if (!showMenu.value && !isDragging.value && currentMood.value === 'neutral') {
      if (Math.random() > 0.98) {
        showRandomSpeech()
      }
    }
  }, 1000)
})

onUnmounted(() => {
  document.removeEventListener('mousemove', onDrag)
  document.removeEventListener('mouseup', stopDrag)
  if (speechInterval) {
    clearInterval(speechInterval)
    speechInterval = null
  }
  moodTimeouts.forEach(timeout => clearTimeout(timeout))
  moodTimeouts = []
})
</script>

<style scoped>
.floating-pet {
  position: fixed;
  top: 0;
  left: 0;
  z-index: 9999;
  cursor: grab;
  transition: opacity 0.3s ease;
}

.floating-pet.dragging {
  cursor: grabbing;
}

.pet-toggle {
  position: absolute;
  top: -25px;
  right: -5px;
  width: 24px;
  height: 24px;
  background: linear-gradient(135deg, #fbbf24, #8b5cf6);
  border-radius: 50%;
  color: white;
  font-size: 10px;
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.2);
  opacity: 0.7;
  transition: opacity 0.2s;
}

.pet-toggle:hover {
  opacity: 1;
}

.pet-menu {
  position: absolute;
  top: -30px;
  right: 30px;
  background: white;
  border-radius: 12px;
  box-shadow: 0 10px 30px rgba(0, 0, 0, 0.2);
  padding: 8px;
  min-width: 80px;
  z-index: 10000;
}

.menu-btn {
  display: block;
  width: 100%;
  padding: 6px 12px;
  border: none;
  background: transparent;
  color: #0f172a;
  font-size: 12px;
  text-align: left;
  cursor: pointer;
  border-radius: 6px;
  transition: background 0.2s;
}

.menu-btn:hover {
  background: #f1f5f9;
}

.menu-divider {
  height: 1px;
  background: #e2e8f0;
  margin: 6px 0;
}

.floating-pet svg {
  width: 90px;
  height: 80px;
  filter: drop-shadow(0 4px 12px rgba(0, 0, 0, 0.15));
  transition: transform 0.2s ease;
}

.floating-pet:hover svg {
  transform: scale(1.1);
}

.floating-pet.happy svg {
  animation: float-bounce 0.5s infinite;
}

.floating-pet.excited svg {
  animation: float-jump 0.4s infinite;
}

.floating-pet.sleep svg {
  animation: float-nod 3s infinite ease-in-out;
}

@keyframes float-bounce {
  0%, 100% { transform: translateY(0); }
  50% { transform: translateY(-5px); }
}

@keyframes float-jump {
  0%, 100% { transform: translateY(0) rotate(0deg); }
  25% { transform: translateY(-8px) rotate(-3deg); }
  75% { transform: translateY(-8px) rotate(3deg); }
}

@keyframes float-nod {
  0%, 100% { transform: rotate(0deg); }
  25% { transform: rotate(-3deg); }
  75% { transform: rotate(3deg); }
}

.speech-bubble {
  position: absolute;
  top: -45px;
  right: -10px;
  background: white;
  padding: 6px 12px;
  border-radius: 12px;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
  font-size: 12px;
  font-weight: 500;
  color: #0f172a;
  white-space: nowrap;
  animation: speech-pop 0.3s ease;
}

.speech-bubble::after {
  content: '';
  position: absolute;
  bottom: -6px;
  left: 15px;
  border-width: 6px 6px 0;
  border-style: solid;
  border-color: white transparent transparent transparent;
}

@keyframes speech-pop {
  0% {
    opacity: 0;
    transform: scale(0.8);
  }
  100% {
    opacity: 1;
    transform: scale(1);
  }
}
</style>