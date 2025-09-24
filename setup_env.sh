#!/bin/bash

pyenv local 3.11.9

python3 -m venv .venv
source .venv/bin/activate

python -V
pip install -U pip
pip install meson ninja

which python
which meson
which ninja

PY="$(python -c 'import sys; print(sys.executable)')"
cat > native-3.11.ini <<EOF
[binaries]
python = '$PY'
python3 = '$PY'
EOF
cat native-3.11.ini

python -V
python -c 'import sys; print(sys.executable)'
pip install --upgrade pip meson ninja
