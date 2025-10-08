#!/bin/bash
# Spuštění všech testů pro mydiff
# --------------------------------
# Předpoklad: binárka mydiff je o adresář výše (../mydiff)

MYDIFF="./mydiff"

if [ ! -x "$MYDIFF" ]; then
  echo "Chyba: nenalezen spustitelný soubor $MYDIFF"
  exit 1
fi

run_test() {
  local name="$1"
  shift
  echo
  echo "========================================"
  echo ">>> Spouštím test: $name"
  echo "========================================"
  # spustíme mydiff na pozadí
  $MYDIFF "$@" &
  PID=$!
  sleep 2
}

finish_test() {
  sleep 2
  kill $PID 2>/dev/null || true
  wait $PID 2>/dev/null || true
  echo ">>> Test dokončen"
  echo
  echo
  sleep 1
}

# 1. Základní porovnání
echo "Hello" > f1.txt
echo "Hello" > f2.txt
run_test "Základní porovnání" f1.txt f2.txt
echo "World" >> f1.txt
finish_test

# 2. Zkrácení souboru
echo -e "A\nB\nC" > f2.txt
cp f2.txt f1.txt
run_test "Zkrácení souboru" f1.txt f2.txt
truncate -s 0 f2.txt
finish_test

# 3. Změna práv
echo "X" > f1.txt
echo "X" > f2.txt
chmod 644 f1.txt f2.txt
run_test "Změna práv" f1.txt f2.txt
chmod 755 f1.txt
finish_test

# 4. Změna času modifikace
echo "X" > f1.txt
echo "X" > f2.txt
run_test "Změna času modifikace (-t)" -t f1.txt f2.txt
touch f2.txt
finish_test

# 5. Kombinace změn
echo "Init" > f1.txt
echo "Init" > f2.txt
chmod 644 f1.txt f2.txt
run_test "Kombinace změn (-s -t)" -s -t f1.txt f2.txt
echo "New line" >> f1.txt
chmod 600 f1.txt
truncate -s 0 f2.txt
finish_test

# 6. Zmizení souboru
echo "X" > f1.txt
echo "X" > f2.txt
run_test "Zmizení souboru" f1.txt f2.txt
rm f2.txt
finish_test

# 7. Stress test
seq 1 1000 > f1.txt
seq 1 1000 > f2.txt
run_test "Stress test" f1.txt f2.txt
echo "1001" >> f1.txt
chmod 777 f2.txt
truncate -s 50 f1.txt
finish_test

echo "========================================"
echo " Všechny testy dokončeny "
echo "========================================"