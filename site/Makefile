# Os indicadores desta proposta são MEDIDOS, nunca digitados (ordem do Diego,
# 2026-07-12): a página afirmava números que ninguém conseguia reproduzir.
# O medidor mora no repo da ferramenta, que é quem tem a suíte e o corpus.
#
#   make numbers   remede e reescreve os indicadores
#   make check     falha se algum estiver defasado
HBREFACTOR ?= $(HOME)/devel/hbrefactor

numbers:
	@CORE=$(CURDIR)/.. $(HBREFACTOR)/bin/site-numbers.sh

check:
	@CORE=$(CURDIR)/.. $(HBREFACTOR)/bin/site-numbers.sh --check

.PHONY: numbers check
