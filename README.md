**Iskrembod-simulering – Multithreading og semaforer (C)**

Dette prosjektet simulerer en iskrembod der flere arbeidere og kunder samhandler samtidig gjennom bruk av pthread og semaforer for synkronisering.
Programmet modellerer en begrenset kø, arbeidere som tar pauser, og kunder som enten slapper av eller forsøker å få is.

**Flere kunde-tråder som:**
  * slapper av i tilfeldig tid
  * forsøker å stille seg i kø
  * venter på å bli betjent

**Flere arbeider-tråder som:**
  * venter på kunder
  * serverer is
  * tar obligatoriske pauser etter et visst antall serveringer

**Semaforer brukes for:**
  * kø-lås
  * signaler om kunder som venter
  * signaler om ferdig servering

Programmet avsluttes automatisk etter 60 sekunder
