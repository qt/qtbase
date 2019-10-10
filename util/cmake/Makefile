
test: flake8 mypy pytest

coverage:
	pytest --cov .

flake8:
	flake8 *.py --ignore=E501,E266,E203,W503

pytest:
	pytest

mypy:
	mypy --pretty *.py
