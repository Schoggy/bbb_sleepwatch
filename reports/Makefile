
PYTHON = python2
RST2HTML = rst2html
RST2LATEX = rst2latex
PDFLATEX = pdflatex
RM = rm -f

usage:
	@echo "make html | pdf | clean "

all: bericht.html bericht.pdf

pdf: bericht.pdf

html: bericht.html

bericht.html: etc/kopf.rst bericht.rst titelseite.html titelseite.tex bericht.cfg
	$(RST2HTML) --language=de --stylesheet=etc/goodger.css bericht.rst bericht.html

bericht.pdf: etc/kopf.rst bericht.rst titelseite.tex titelseite.html bericht.cfg
	$(RST2LATEX) --language=de --use-latex-citation --stylesheet=etc/pygments-docutilsroles.sty bericht.rst bericht.tex
	$(PDFLATEX) bericht.tex
	$(PDFLATEX) bericht.tex

titelseite.tex: etc/titelseite.tex.in
	$(PYTHON) etc/em.py etc/titelseite.tex.in > titelseite.tex

titelseite.html: etc/titelseite.html.in
	$(PYTHON) etc/em.py etc/titelseite.html.in > titelseite.html

etc/kopf.rst: etc/kopf.rst.in
	$(PYTHON) etc/em.py etc/kopf.rst.in > etc/kopf.rst

clean:
	$(RM) *.aux *.toc *.log *.out *.tex titelseite.html etc/kopf.rst *~

veryclean: clean
	$(RM) bericht.html bericht.pdf

