
test: flake8 mypy pytest black_format_check

coverage:
	pytest --cov .

format:
	black *.py --line-length 100

black_format_check:
	black *.py --line-length 100 --check

flake8:
	flake8 *.py --ignore=E501,E266,E203,W503,F541

pytest:
	pytest

mypy:
	mypy --pretty *.py
