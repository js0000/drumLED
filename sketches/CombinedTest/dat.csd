; dat.csd
; Drum Amplitude Test
; like a bass drum getting progressively louder, then quiter
<CsoundSynthesizer>
  <CsOptions>
    --format=flac
    --output=dat.flac
  </CsOptions>

  <CsInstruments>
    sr = 44100
    ksmps = 32
    nchnls = 2
    0dbfs  = 1

    instr 1
      kamp = p4
      kcps = 64

      kenv adsr 0.01, 0.1, 0, 0.01
      asig oscil kenv * kamp, kcps
           outs asig, asig
    endin
  </CsInstruments>

  <CsScore>
    i1  0 0.25 0.1
    i1  2 0.25 0.3
    i1  4 0.25 0.5
    i1  6 0.25 0.7
    i1  8 0.25 1.0
    i1 10 0.25 0.8
    i1 12 0.25 0.6
    i1 14 0.25 0.4
    i1 16 0.25 0.2
    ; keep rhythm for loop
    i1 17.9 0.1 0.0
    e
  </CsScore>
</CsoundSynthesizer>
