./run setup
./run python -- -m build
./run pip install dist/pycircularbuffer-0.0.9.tar.gz
./run pybin pytest --capture=no
