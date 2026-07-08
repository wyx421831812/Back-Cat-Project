<template>
  <svg class="pet-avatar" :class="sizeClass" viewBox="0 0 200 180" xmlns="http://www.w3.org/2000/svg">
    <defs>
      <linearGradient :id="hairGradientId" x1="0%" y1="0%" x2="100%" y2="100%">
        <stop offset="0%" stop-color="#e8e8ed"/>
        <stop offset="30%" stop-color="#c8c8d2"/>
        <stop offset="60%" stop-color="#a8a8b8"/>
        <stop offset="100%" stop-color="#6b6b7b"/>
      </linearGradient>
      <linearGradient :id="hairDarkGradientId" x1="0%" y1="0%" x2="100%" y2="100%">
        <stop offset="0%" stop-color="#888898"/>
        <stop offset="100%" stop-color="#4a4a5a"/>
      </linearGradient>
      <linearGradient :id="skinGradientId" x1="0%" y1="0%" x2="0%" y2="100%">
        <stop offset="0%" stop-color="#fce4d6"/>
        <stop offset="100%" stop-color="#f5d4c0"/>
      </linearGradient>
      <linearGradient :id="eyeGradientId" x1="0%" y1="0%" x2="100%" y2="100%">
        <stop offset="0%" stop-color="#4fc3f7"/>
        <stop offset="30%" stop-color="#29b6f6"/>
        <stop offset="70%" stop-color="#03a9f4"/>
        <stop offset="100%" stop-color="#0288d1"/>
      </linearGradient>
      <filter :id="shadowId">
        <feDropShadow dx="2" dy="2" stdDeviation="2" flood-opacity="0.3"/>
      </filter>
    </defs>
    
    <g class="hair-back">
      <path d="M30 120 Q20 140 35 155 Q50 170 70 165 L85 160 Q90 165 100 162 Q110 165 120 160 L135 165 Q155 170 170 155 Q185 140 175 120" :fill="`url(#${hairDarkGradientId})`" opacity="0.6"/>
      <path d="M25 110 Q15 130 30 145 Q45 160 65 155 L80 150 Q85 155 95 152 Q105 155 115 150 L130 155 Q150 160 165 145 Q180 130 170 110" :fill="`url(#${hairGradientId})`"/>
    </g>
    
    <ellipse cx="100" cy="110" rx="75" ry="55" :fill="`url(#${skinGradientId})`" :filter="`url(#${shadowId})`"/>
    
    <g class="hair-front">
      <path d="M50 50 Q40 20 65 15 Q85 10 95 30 L90 50 Q80 40 70 55 Q55 70 50 50" :fill="`url(#${hairGradientId})`"/>
      <path d="M95 30 Q100 5 115 15 Q135 20 145 40 Q155 60 140 70 L125 55 Q115 45 105 50 Q95 55 90 50 Z" :fill="`url(#${hairGradientId})`"/>
      <path d="M70 55 Q80 45 90 50 Q100 55 105 50 Q115 45 125 55 L130 70 Q120 65 100 75 Q80 65 65 70 Z" :fill="`url(#${hairDarkGradientId})`" opacity="0.5"/>
      <path d="M45 75 Q35 55 50 50 Q65 45 75 60 Q80 70 70 75 Q60 80 45 75" :fill="`url(#${hairGradientId})`"/>
      <path d="M150 70 Q165 50 155 40 Q145 30 135 45 Q125 60 130 70" :fill="`url(#${hairGradientId})`"/>
      <path d="M155 40 Q175 25 185 40 Q195 55 175 60 Q165 65 155 55 Z" :fill="`url(#${hairGradientId})`"/>
      <path d="M65 15 Q50 5 45 20 Q40 35 55 40 Q65 45 65 15" :fill="`url(#${hairGradientId})`"/>
      <path d="M145 15 Q165 5 170 20 Q175 35 160 40 Q150 45 145 15" :fill="`url(#${hairGradientId})`"/>
      <path d="M85 45 Q80 30 95 25 Q110 30 105 45 Q100 50 90 50 Z" :fill="`url(#${hairDarkGradientId})`" opacity="0.4"/>
    </g>
    
    <g class="eyebrows" :transform="eyebrowTransform">
      <path d="M60 70 Q70 65 85 70" stroke="#6b6b7b" stroke-width="3" fill="none" stroke-linecap="round"/>
      <path d="M115 70 Q130 65 140 70" stroke="#6b6b7b" stroke-width="3" fill="none" stroke-linecap="round"/>
    </g>
    
    <g class="eyes" :transform="eyeTransform">
      <ellipse cx="70" cy="85" rx="18" ry="15" fill="white" :filter="`url(#${shadowId})`"/>
      <ellipse cx="130" cy="85" rx="18" ry="15" fill="white" :filter="`url(#${shadowId})`"/>
      
      <g class="iris" :transform="pupilTransform">
        <ellipse cx="70" cy="85" rx="12" ry="10" :fill="`url(#${eyeGradientId})`"/>
        <ellipse cx="130" cy="85" rx="12" ry="10" :fill="`url(#${eyeGradientId})`"/>
        <ellipse cx="67" cy="82" rx="4" ry="3" fill="white" opacity="0.8"/>
        <ellipse cx="127" cy="82" rx="4" ry="3" fill="white" opacity="0.8"/>
        <circle cx="70" cy="85" r="3" fill="#0d47a1"/>
        <circle cx="130" cy="85" r="3" fill="#0d47a1"/>
      </g>
      
      <path d="M55 80 Q52 78 50 80" stroke="#333" stroke-width="1.5" fill="none"/>
      <path d="M60 78 Q58 75 62 78" stroke="#333" stroke-width="1.5" fill="none"/>
      <path d="M65 78 Q65 75 68 78" stroke="#333" stroke-width="1.5" fill="none"/>
      <path d="M135 78 Q135 75 138 78" stroke="#333" stroke-width="1.5" fill="none"/>
      <path d="M140 78 Q142 75 140 80" stroke="#333" stroke-width="1.5" fill="none"/>
      <path d="M145 80 Q148 78 150 80" stroke="#333" stroke-width="1.5" fill="none"/>
      
      <path d="M58 95 Q56 98 60 96" stroke="#333" stroke-width="1" fill="none" opacity="0.5"/>
      <path d="M65 96 Q65 99 68 97" stroke="#333" stroke-width="1" fill="none" opacity="0.5"/>
      <path d="M132 96 Q132 99 135 97" stroke="#333" stroke-width="1" fill="none" opacity="0.5"/>
      <path d="M142 95 Q144 98 140 96" stroke="#333" stroke-width="1" fill="none" opacity="0.5"/>
    </g>
    
    <ellipse cx="100" cy="105" rx="6" ry="4" fill="#d4a5a5" opacity="0.6"/>
    
    <path class="mouth" :d="mouthPath" stroke="#c49595" stroke-width="2" fill="none" stroke-linecap="round"/>
    
    <path d="M45 95 Q40 100 45 105 Q50 100 45 95" fill="#ffcdd2" opacity="0.4"/>
    <path d="M155 95 Q160 100 155 105 Q150 100 155 95" fill="#ffcdd2" opacity="0.4"/>
  </svg>
</template>

<script setup>
import { computed } from 'vue'

const props = defineProps({
  mood: {
    type: String,
    default: 'neutral'
  },
  size: {
    type: String,
    default: 'medium',
    validator: (val) => ['small', 'medium', 'large'].includes(val)
  }
})

const gradientId = Math.random().toString(36).substr(2, 9)
const hairGradientId = `hairGradient${gradientId}`
const hairDarkGradientId = `hairDarkGradient${gradientId}`
const skinGradientId = `skinGradient${gradientId}`
const eyeGradientId = `eyeGradient${gradientId}`
const shadowId = `shadow${gradientId}`

const sizeClass = computed(() => `pet-size-${props.size}`)

const eyebrowTransform = computed(() => {
  if (props.mood === 'happy') return 'translate(0, -3)'
  if (props.mood === 'excited') return 'translate(0, -5)'
  if (props.mood === 'sleep') return 'translate(0, 5)'
  return 'translate(0, 0)'
})

const eyeTransform = computed(() => {
  if (props.mood === 'sleep') return 'translate(0, 5)'
  return 'translate(0, 0)'
})

const pupilTransform = computed(() => {
  if (props.mood === 'sleep') return 'translate(0, 3)'
  if (props.mood === 'happy') return 'translate(0, -2)'
  return 'translate(0, 0)'
})

const mouthPath = computed(() => {
  if (props.mood === 'happy') return 'M85 120 Q100 135 115 120'
  if (props.mood === 'excited') return 'M80 120 Q100 140 120 120'
  if (props.mood === 'sleep') return 'M90 122 Q100 124 110 122'
  return 'M90 122 Q100 125 110 122'
})
</script>

<style scoped>
.pet-avatar {
  transition: transform 0.2s ease;
}

.pet-size-small {
  width: 60px;
  height: 55px;
}

.pet-size-medium {
  width: 100px;
  height: 90px;
}

.pet-size-large {
  width: 180px;
  height: 160px;
}
</style>