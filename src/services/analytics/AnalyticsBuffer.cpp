// =============================================================
//  Project SIGMA
//  Arquivo    : AnalyticsBuffer.cpp
//  Descricao  : Implementacao do Circular Buffer analitico.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.10.0
//  Codename   : Event Manager
//  Data       : 2026-06-28
// =============================================================

#include "AnalyticsBuffer.h"

// =============================================================
//  CONSTRUTOR
// =============================================================
AnalyticsBuffer::AnalyticsBuffer() {
  clear();
}

// =============================================================
//  FUNCAO: addSample
//  Insere amostra. Sobrescreve a mais antiga se estiver cheio.
// =============================================================
void AnalyticsBuffer::addSample(const AnalyticsSample& sample) {
  _buffer[_head] = sample;
  _head = (_head + 1) % ANALYTICS_BUFFER_SIZE;
  
  if (_count < ANALYTICS_BUFFER_SIZE) {
    _count++;
  } else {
    // Buffer cheio: avanca o tail, perdendo dado mais velho
    _tail = (_tail + 1) % ANALYTICS_BUFFER_SIZE;
  }
}

// =============================================================
//  FUNCAO: clear
//  Zera os ponteiros lógicos. A memoria antiga e ignorada.
// =============================================================
void AnalyticsBuffer::clear() {
  _head  = 0;
  _tail  = 0;
  _count = 0;
}

int AnalyticsBuffer::size() const {
  return _count;
}

int AnalyticsBuffer::capacity() const {
  return ANALYTICS_BUFFER_SIZE;
}

bool AnalyticsBuffer::isFull() const {
  return (_count == ANALYTICS_BUFFER_SIZE);
}

bool AnalyticsBuffer::isEmpty() const {
  return (_count == 0);
}

// =============================================================
//  FUNCAO: get
//  Recupera amostra por indice logico (0 = mais antiga).
// =============================================================
bool AnalyticsBuffer::get(int index, AnalyticsSample& outSample) const {
  if (index < 0 || index >= _count) return false;
  
  // Mapeia o indice logico para a posicao fisica no anel
  int posicaoFisica = (_tail + index) % ANALYTICS_BUFFER_SIZE;
  outSample = _buffer[posicaoFisica];
  return true;
}

// =============================================================
//  FUNCAO: latest
//  Retorna a ultima amostra inserida.
// =============================================================
bool AnalyticsBuffer::latest(AnalyticsSample& outSample) const {
  if (isEmpty()) return false;
  
  // O ultimo item esta uma posicao antes do head
  int pos = (_head - 1 + ANALYTICS_BUFFER_SIZE) % ANALYTICS_BUFFER_SIZE;
  outSample = _buffer[pos];
  return true;
}