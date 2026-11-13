// VCO quantizado para Arduino Uno com Mozzi 2.x
//
// A0  potenciometro de nota (transpositor de 4 oitavas)
// A1  entrada de CV 1V/oct (0 a 5V, com 1k, BAT85 x2, 100n e 100k)
// D2  botao de forma de onda (para o GND)
// D3  botao de quantizacao (para o GND)
// D13  LED de quantizacao
// D4, D5..D8, D10..D12  LEDs das 8 formas de onda
// D9  saida de audio (filtro RC + capacitor de acoplamento)

#define MOZZI_CONTROL_RATE 128
#include <Mozzi.h>
#include <Oscil.h>
#include <WaveShaper.h>
#include <tables/sin2048_int8.h>
#include <tables/triangle2048_int8.h>
#include <tables/saw2048_int8.h>
#include <tables/square_no_alias_2048_int8.h>
#include <tables/saw_analogue512_int8.h>
#include <tables/waveshape_chebyshev_3rd_256_int8.h>

// ---------- pinos ----------
const uint8_t PINO_POT = A0;
const uint8_t PINO_CV = A1;
const uint8_t PINO_BOTAO_ONDA = 2;
const uint8_t PINO_BOTAO_QUANT = 3;
const uint8_t PINO_LED_QUANT = 13;
const uint8_t LEDS_ONDA[] = {4, 5, 6, 7, 8, 10, 11, 12};

// ---------- calibracao ----------
const float VREF = 5.0;                // tensao real do pino 5V (meça com multimetro)
const float VOLTS_POR_OITAVA = 1.0;    // ajuste fino da escala do CV
const float NOTA_BASE = 24.0;          // nota MIDI com pot e CV em zero (C1)
const float FAIXA_POT = 48.0;          // semitons percorridos pelo potenciometro

// ---------- escala (semitons a partir da tonica) ----------
// Pentatonica menor. Outras: maior {0,2,4,5,7,9,11}, cromatica {0..11}
const uint8_t ESCALA[] = {0, 3, 5, 7, 10};
const uint8_t TAM_ESCALA = sizeof(ESCALA);
int8_t ajusteEscala[12];               // semitom -> deslocamento ate a nota mais proxima

// ---------- formas de onda ----------
enum Modo { SERRA, TRIANGULO, QUADRADA, SENOIDE, SERRA_ANALOGICA, SUPERSAW, SINO, VIDRO, NUM_MODOS };

Oscil<2048, MOZZI_AUDIO_RATE> oscPrincipal(SAW2048_DATA);
Oscil<512, MOZZI_AUDIO_RATE> oscAnalogica(SAW_ANALOGUE512_DATA);
Oscil<2048, MOZZI_AUDIO_RATE> serra1(SAW2048_DATA);
Oscil<2048, MOZZI_AUDIO_RATE> serra2(SAW2048_DATA);
Oscil<2048, MOZZI_AUDIO_RATE> serra3(SAW2048_DATA);
Oscil<2048, MOZZI_AUDIO_RATE> fmPortadora(SIN2048_DATA);
Oscil<2048, MOZZI_AUDIO_RATE> fmModuladora(SIN2048_DATA);
WaveShaper<char> chebyshev(CHEBYSHEV_3RD_256_DATA);

const float DESAFINO_SUPERSAW = 0.007; // 0,7% entre as serras
const float RAZAO_SINO = 3.5;          // razao nao inteira = som metalico
const int32_t SINO_PROFUNDIDADE_MAX = 400;
const int32_t SINO_PROFUNDIDADE_MIN = 20;

// ---------- estado ----------
uint8_t modo = SERRA;
bool quantizar = true;
float potFiltrado = 0, cvFiltrado = 0;
float notaQuantizada = -100;
float notaUltimoDisparo = -100;
int32_t sinoProfundidade = 0;

struct Botao {
  uint8_t pino;
  uint8_t estavel;
  uint8_t contagem;
};
Botao botaoOnda = {PINO_BOTAO_ONDA, HIGH, 0};
Botao botaoQuant = {PINO_BOTAO_QUANT, HIGH, 0};

// Retorna true uma vez por aperto (com debounce de 3 ciclos de controle)
bool apertou(Botao &b) {
  uint8_t leitura = digitalRead(b.pino);
  if (leitura != b.estavel) {
    if (++b.contagem >= 3) {
      b.estavel = leitura;
      b.contagem = 0;
      return leitura == LOW;
    }
  } else {
    b.contagem = 0;
  }
  return false;
}

void montarEscala() {
  for (int s = 0; s < 12; s++) {
    int melhor = 0, menorDist = 99;
    for (int i = 0; i <= TAM_ESCALA; i++) {
      int grau = (i < TAM_ESCALA) ? ESCALA[i] : ESCALA[0] + 12;
      int dist = abs(grau - s);
      if (dist < menorDist) { menorDist = dist; melhor = grau; }
    }
    ajusteEscala[s] = melhor - s;
  }
}

int quantizarNota(float semitons) {
  int n = (int)(semitons + 0.5);
  return n + ajusteEscala[n % 12];
}

void aplicarModo() {
  switch (modo) {
    case SERRA:     oscPrincipal.setTable(SAW2048_DATA); break;
    case TRIANGULO: oscPrincipal.setTable(TRIANGLE2048_DATA); break;
    case QUADRADA:  oscPrincipal.setTable(SQUARE_NO_ALIAS_2048_DATA); break;
    case SENOIDE:
    case VIDRO:     oscPrincipal.setTable(SIN2048_DATA); break;
    default: break;
  }
  for (uint8_t i = 0; i < NUM_MODOS; i++) digitalWrite(LEDS_ONDA[i], i == modo ? HIGH : LOW);
}

void setup() {
  pinMode(PINO_BOTAO_ONDA, INPUT_PULLUP);
  pinMode(PINO_BOTAO_QUANT, INPUT_PULLUP);
  pinMode(PINO_LED_QUANT, OUTPUT);
  for (uint8_t i = 0; i < NUM_MODOS; i++) pinMode(LEDS_ONDA[i], OUTPUT);
  montarEscala();
  aplicarModo();
  digitalWrite(PINO_LED_QUANT, quantizar);
  startMozzi();
}

void updateControl() {
  // Botoes
  if (apertou(botaoOnda)) {
    modo = (modo + 1) % NUM_MODOS;
    aplicarModo();
  }
  if (apertou(botaoQuant)) {
    quantizar = !quantizar;
    digitalWrite(PINO_LED_QUANT, quantizar);
  }

  // Leituras com um filtro simples para reduzir ruido
  potFiltrado += (mozziAnalogRead<10>(PINO_POT) - potFiltrado) * 0.25;
  cvFiltrado += (mozziAnalogRead<10>(PINO_CV) - cvFiltrado) * 0.25;

  float semitons = NOTA_BASE
                 + potFiltrado * (FAIXA_POT / 1023.0)
                 + cvFiltrado * (VREF / 1023.0) * (12.0 / VOLTS_POR_OITAVA);

  float nota;
  if (quantizar) {
    // Histerese: so troca de nota quando a nova estiver claramente mais perto
    int candidata = quantizarNota(semitons);
    if (candidata != notaQuantizada &&
        fabs(semitons - notaQuantizada) - fabs(semitons - candidata) > 0.3) {
      notaQuantizada = candidata;
    }
    nota = notaQuantizada;
  } else {
    nota = semitons;
    notaQuantizada = -100;
  }

  // Sino: redispara o brilho a cada nota nova e deixa decair
  if (fabs(nota - notaUltimoDisparo) >= 0.5) {
    sinoProfundidade = SINO_PROFUNDIDADE_MAX;
    notaUltimoDisparo = nota;
  }
  if (sinoProfundidade > SINO_PROFUNDIDADE_MIN) sinoProfundidade = (sinoProfundidade * 31) >> 5;

  float freq = 440.0 * pow(2.0, (nota - 69.0) / 12.0);
  oscPrincipal.setFreq(freq);
  oscAnalogica.setFreq(freq);
  serra1.setFreq(freq);
  serra2.setFreq(freq * (1.0f + DESAFINO_SUPERSAW));
  serra3.setFreq(freq * (1.0f - DESAFINO_SUPERSAW));
  fmPortadora.setFreq(freq);
  fmModuladora.setFreq(freq * RAZAO_SINO);
}

AudioOutput updateAudio() {
  switch (modo) {
    case SERRA_ANALOGICA:
      return MonoOutput::from8Bit(oscAnalogica.next());
    case SUPERSAW: {
      int soma = serra1.next() + serra2.next() + serra3.next();
      return MonoOutput::fromNBit(10, soma);
    }
    case SINO: {
      int32_t modulacao = (int32_t)fmModuladora.next() * sinoProfundidade;
      return MonoOutput::from8Bit(fmPortadora.phMod(modulacao));
    }
    case VIDRO: {
      // Senoide a ~70% passando pelo polinomio de Chebyshev: fundamental + 3o harmonico
      int8_t s = oscPrincipal.next();
      return MonoOutput::from8Bit(chebyshev.next((uint8_t)(128 + ((s * 45) >> 6))));
    }
    default:
      return MonoOutput::from8Bit(oscPrincipal.next());
  }
}

void loop() {
  audioHook();
}
