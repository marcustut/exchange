.PHONY: deps run

deps:
	cargo install dioxus-cli

dev-tailwindcss:
	npx tailwindcss -i ./src/globals.css -o ./public/tailwind.css --watch

dev-desktop:
	dx serve --hot-reload --platform desktop

dev-web:
	dx serve --hot-reload --platform web

