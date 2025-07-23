lint: check format_check

env:
	uv sync

check:
	uvx ruff check
	uvx pyright

format:
	uvx ruff format

format_check:
	uvx ruff format --check
