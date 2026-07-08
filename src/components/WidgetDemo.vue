<template>
  <div class="desktop-demo">
    <div class="desktop-background"></div>
    
    <div class="back-button" @click="$emit('back')">
      <span>←</span> 返回首页
    </div>
    
    <div class="desktop-content">
      <div class="widget-container" :class="{ 'widget-dragging': isDragging }" @mousedown="startDrag">
        <div class="widget-header">
          <div class="widget-title">BackPet</div>
          <div class="widget-controls">
            <span class="control-btn" @click.stop="toggleCollapse">▲</span>
          </div>
        </div>
        
        <div class="widget-body" v-show="!isCollapsed">
          <div class="widget-pet">
            <PetAvatar :mood="currentMood" size="large" />
            <div class="pet-interaction">
              <div class="mood-bubble" v-show="showBubble">{{ bubbleText }}</div>
              <button class="pet-btn" @click.stop="setMood('happy')">😊</button>
              <button class="pet-btn" @click.stop="setMood('excited')">🤪</button>
              <button class="pet-btn" @click.stop="setMood('sleep')">😴</button>
            </div>
          </div>
          
          <div class="widget-info">
            <div class="clock-section">
              <div class="clock-time">{{ currentTime }}</div>
              <div class="clock-date">{{ currentDate }}</div>
            </div>
            
            <div class="weather-section">
              <div class="weather-icon">🌤️</div>
              <div class="weather-info">
                <div class="weather-temp">{{ weather.temperature }}°C</div>
                <div class="weather-condition">{{ weather.condition }}</div>
                <div class="weather-details">
                  <span>湿度 {{ weather.humidity }}%</span>
                  <span>风速 {{ weather.wind }}</span>
                </div>
              </div>
            </div>
            
            <div class="quote-section">
              <div class="quote-icon">💬</div>
              <div class="quote-text">{{ currentQuote }}</div>
            </div>
          </div>
        </div>
      </div>
      
      <div class="desktop-icons">
        <div class="desktop-icon">
          <div class="icon-img">🖥️</div>
          <div class="icon-label">此电脑</div>
        </div>
        <div class="desktop-icon">
          <div class="icon-img">📁</div>
          <div class="icon-label">文档</div>
        </div>
        <div class="desktop-icon">
          <div class="icon-img">🖼️</div>
          <div class="icon-label">图片</div>
        </div>
        <div class="desktop-icon">
          <div class="icon-img">🎵</div>
          <div class="icon-label">音乐</div>
        </div>
        <div class="desktop-icon">
          <div class="icon-img">🐱</div>
          <div class="icon-label">BackPet</div>
        </div>
      </div>
    </div>
    
    <div class="taskbar">
      <div class="start-btn">
        <span>⊞</span>
      </div>
      <div class="taskbar-icons">
        <div class="task-icon active">🖥️</div>
        <div class="task-icon">🐱</div>
        <div class="task-icon">📁</div>
      </div>
      <div class="taskbar-right">
        <span class="tray-icon">🌤️</span>
        <span class="tray-icon">🔊</span>
        <span class="tray-time">{{ currentTime }}</span>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted } from 'vue'
import PetAvatar from './PetAvatar.vue'

defineEmits(['back'])

const currentTime = ref('')
const currentDate = ref('')
const currentMood = ref('neutral')
const isDragging = ref(false)
const dragOffset = ref({ x: 0, y: 0 })
const isCollapsed = ref(false)
const showBubble = ref(false)
const bubbleText = ref('')

const weather = ref({
  temperature: 22,
  condition: '多云',
  humidity: 65,
  wind: '3级'
})

const quotes = [
  '今天也要加油哦！',
  '我陪着你~',
  '休息一下吧~',
  '你真棒！',
  '摸摸头~',
  '保持微笑😊',
  '一切都会好的~',
  '爱你哟！'
]
const currentQuote = ref(quotes[0])

let timeInterval = null
let quoteInterval = null
let autoMoodInterval = null
let moodTimeouts = []

function updateTime() {
  const now = new Date()
  currentTime.value = now.toLocaleTimeString('zh-CN', { hour: '2-digit', minute: '2-digit' })
  currentDate.value = now.toLocaleDateString('zh-CN', { year: 'numeric', month: 'long', day: 'numeric', weekday: 'long' })
}

function startDrag(e) {
  if (e.target.closest('.widget-controls') || e.target.closest('.pet-btn')) return
  
  isDragging.value = true
  dragOffset.value = {
    x: e.clientX - e.currentTarget.getBoundingClientRect().left,
    y: e.clientY - e.currentTarget.getBoundingClientRect().top
  }
  
  document.addEventListener('mousemove', onDrag)
  document.addEventListener('mouseup', stopDrag)
}

function onDrag(e) {
  if (!isDragging.value) return
  
  const widget = document.querySelector('.widget-container')
  if (widget) {
    const maxX = window.innerWidth - widget.offsetWidth - 20
    const maxY = window.innerHeight - widget.offsetHeight - 80
    
    widget.style.left = `${Math.max(20, Math.min(maxX, e.clientX - dragOffset.value.x))}px`
    widget.style.top = `${Math.max(20, Math.min(maxY, e.clientY - dragOffset.value.y))}px`
  }
}

function stopDrag() {
  isDragging.value = false
  document.removeEventListener('mousemove', onDrag)
  document.removeEventListener('mouseup', stopDrag)
}

function toggleCollapse() {
  isCollapsed.value = !isCollapsed.value
}

function setMood(mood) {
  currentMood.value = mood
  
  const texts = {
    happy: '开心鼓掌！',
    excited: '高兴呐喊~',
    sleep: '犯困打盹...'
  }
  bubbleText.value = texts[mood]
  showBubble.value = true
  
  moodTimeouts.push(setTimeout(() => {
    showBubble.value = false
  }, 1500))
  
  moodTimeouts.push(setTimeout(() => {
    if (mood !== 'sleep') {
      currentMood.value = 'neutral'
    }
  }, 3000))
}

onMounted(() => {
  updateTime()
  timeInterval = setInterval(updateTime, 1000)
  
  quoteInterval = setInterval(() => {
    currentQuote.value = quotes[Math.floor(Math.random() * quotes.length)]
  }, 10000)
  
  autoMoodInterval = setInterval(() => {
    if (Math.random() > 0.95 && currentMood.value === 'neutral') {
      const moods = ['happy', 'excited']
      setMood(moods[Math.floor(Math.random() * moods.length)])
    }
  }, 5000)
})

onUnmounted(() => {
  if (timeInterval) {
    clearInterval(timeInterval)
    timeInterval = null
  }
  if (quoteInterval) {
    clearInterval(quoteInterval)
    quoteInterval = null
  }
  if (autoMoodInterval) {
    clearInterval(autoMoodInterval)
    autoMoodInterval = null
  }
  moodTimeouts.forEach(timeout => clearTimeout(timeout))
  moodTimeouts = []
  document.removeEventListener('mousemove', onDrag)
  document.removeEventListener('mouseup', stopDrag)
})
</script>

<style scoped>
.desktop-demo {
  position: fixed;
  top: 0;
  left: 0;
  width: 100vw;
  height: 100vh;
  overflow: hidden;
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
}

.desktop-background {
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background: linear-gradient(135deg, #1a1a2e 0%, #16213e 50%, #0f3460 100%);
  background-size: cover;
  background-position: center;
}

.desktop-background::before {
  content: '';
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background: 
    radial-gradient(circle at 20% 20%, rgba(251, 191, 36, 0.1) 0%, transparent 50%),
    radial-gradient(circle at 80% 80%, rgba(139, 92, 246, 0.1) 0%, transparent 50%),
    radial-gradient(circle at 50% 50%, rgba(99, 102, 241, 0.05) 0%, transparent 70%);
}

.back-button {
  position: absolute;
  top: 20px;
  left: 20px;
  z-index: 100;
  background: rgba(255, 255, 255, 0.15);
  backdrop-filter: blur(10px);
  padding: 10px 20px;
  border-radius: 8px;
  color: white;
  cursor: pointer;
  font-size: 14px;
  transition: all 0.3s ease;
  border: 1px solid rgba(255, 255, 255, 0.2);
}

.back-button:hover {
  background: rgba(255, 255, 255, 0.25);
  transform: translateX(-2px);
}

.desktop-content {
  position: relative;
  z-index: 10;
  height: calc(100vh - 48px);
}

.widget-container {
  position: absolute;
  top: 100px;
  left: 100px;
  width: 320px;
  background: rgba(30, 30, 45, 0.85);
  backdrop-filter: blur(20px);
  border-radius: 16px;
  border: 1px solid rgba(255, 255, 255, 0.1);
  box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);
  cursor: grab;
  transition: transform 0.2s ease, box-shadow 0.2s ease;
  overflow: hidden;
}

.widget-container:hover {
  box-shadow: 0 25px 80px rgba(0, 0, 0, 0.4);
}

.widget-container.widget-dragging {
  cursor: grabbing;
  transform: scale(1.02);
}

.widget-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 12px 16px;
  background: rgba(255, 255, 255, 0.05);
  border-bottom: 1px solid rgba(255, 255, 255, 0.1);
}

.widget-title {
  color: white;
  font-weight: 600;
  font-size: 14px;
  display: flex;
  align-items: center;
  gap: 8px;
}

.widget-controls {
  display: flex;
  gap: 8px;
}

.control-btn {
  width: 24px;
  height: 24px;
  border-radius: 50%;
  background: rgba(255, 255, 255, 0.1);
  color: white;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 10px;
  cursor: pointer;
  transition: background 0.2s;
}

.control-btn:hover {
  background: rgba(255, 255, 255, 0.2);
}

.widget-body {
  padding: 16px;
}

.widget-pet {
  text-align: center;
  margin-bottom: 16px;
}

.pet-interaction {
  margin-top: 12px;
}

.pet-btn {
  width: 40px;
  height: 40px;
  border: none;
  border-radius: 50%;
  background: rgba(255, 255, 255, 0.1);
  font-size: 18px;
  cursor: pointer;
  margin: 0 4px;
  transition: all 0.2s ease;
}

.pet-btn:hover {
  background: rgba(255, 255, 255, 0.2);
  transform: scale(1.1);
}

.mood-bubble {
  display: inline-block;
  background: white;
  padding: 6px 12px;
  border-radius: 12px;
  font-size: 12px;
  color: #333;
  margin-bottom: 12px;
  animation: bubble-pop 0.3s ease;
}

@keyframes bubble-pop {
  0% { opacity: 0; transform: scale(0.8); }
  100% { opacity: 1; transform: scale(1); }
}

.widget-info {
  border-top: 1px solid rgba(255, 255, 255, 0.1);
  padding-top: 16px;
}

.clock-section {
  text-align: center;
  margin-bottom: 16px;
}

.clock-time {
  font-size: 36px;
  font-weight: 300;
  color: white;
  letter-spacing: 2px;
}

.clock-date {
  font-size: 12px;
  color: rgba(255, 255, 255, 0.6);
  margin-top: 4px;
}

.weather-section {
  display: flex;
  align-items: center;
  gap: 12px;
  background: rgba(251, 191, 36, 0.1);
  padding: 12px;
  border-radius: 12px;
  margin-bottom: 12px;
}

.weather-icon {
  font-size: 32px;
}

.weather-info {
  flex: 1;
}

.weather-temp {
  font-size: 24px;
  font-weight: 600;
  color: white;
}

.weather-condition {
  font-size: 13px;
  color: rgba(255, 255, 255, 0.8);
}

.weather-details {
  display: flex;
  gap: 12px;
  font-size: 11px;
  color: rgba(255, 255, 255, 0.5);
  margin-top: 4px;
}

.quote-section {
  display: flex;
  align-items: flex-start;
  gap: 8px;
  background: rgba(139, 92, 246, 0.1);
  padding: 12px;
  border-radius: 12px;
}

.quote-icon {
  font-size: 18px;
}

.quote-text {
  flex: 1;
  font-size: 13px;
  color: rgba(255, 255, 255, 0.8);
  line-height: 1.5;
}

.desktop-icons {
  position: absolute;
  top: 20px;
  left: 20px;
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.desktop-icon {
  display: flex;
  flex-direction: column;
  align-items: center;
  width: 80px;
  padding: 8px;
  border-radius: 8px;
  cursor: pointer;
  transition: background 0.2s;
}

.desktop-icon:hover {
  background: rgba(255, 255, 255, 0.1);
}

.icon-img {
  font-size: 32px;
  margin-bottom: 4px;
}

.icon-label {
  font-size: 11px;
  color: white;
  text-align: center;
  text-shadow: 0 1px 3px rgba(0, 0, 0, 0.5);
}

.taskbar {
  position: fixed;
  bottom: 0;
  left: 0;
  width: 100%;
  height: 48px;
  background: rgba(20, 20, 30, 0.9);
  backdrop-filter: blur(10px);
  display: flex;
  align-items: center;
  padding: 0 8px;
  gap: 8px;
  border-top: 1px solid rgba(255, 255, 255, 0.1);
}

.start-btn {
  width: 40px;
  height: 40px;
  border-radius: 8px;
  background: rgba(255, 255, 255, 0.1);
  display: flex;
  align-items: center;
  justify-content: center;
  color: white;
  font-size: 20px;
  cursor: pointer;
  transition: background 0.2s;
}

.start-btn:hover {
  background: rgba(255, 255, 255, 0.2);
}

.taskbar-icons {
  display: flex;
  gap: 4px;
}

.task-icon {
  width: 40px;
  height: 40px;
  border-radius: 8px;
  background: rgba(255, 255, 255, 0.05);
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 18px;
  cursor: pointer;
  transition: background 0.2s;
}

.task-icon:hover {
  background: rgba(255, 255, 255, 0.15);
}

.task-icon.active {
  background: rgba(251, 191, 36, 0.3);
}

.taskbar-right {
  margin-left: auto;
  display: flex;
  align-items: center;
  gap: 12px;
  color: white;
  font-size: 13px;
}

.tray-icon {
  font-size: 14px;
}

.tray-time {
  padding: 8px 12px;
  border-radius: 8px;
  background: rgba(255, 255, 255, 0.05);
}
</style>