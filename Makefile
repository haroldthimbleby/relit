# Simple makefile for relit
# Make and Latex are a mess, and make+latex+relit is worse :-)
# but fortunately most of the steps are pretty fast, so a simple approach is better 

.PHONY: all man zip cleanlatex clean 

all: relit uncom
	./relit relit.tex
	# echo "\\\\\\\\" `date` >> TeX-mode-demo.tex
	javac ForLoop.java
	java ForLoop > java-output.tex
	expr `cat relit.c|wc -l` / 10 "*" 10 > linesofrelit.tex
	diff randomised-euler.c euler.c | grep -B 3 "walk a cycle" | sed "s/$$/\\\\\\\\/" > diff.txt
	./uncom < euler.c | wc -l > t
	@echo euler.c has `cat t` lines of code
	@echo "\\linesofcode"=`cat t` > linesofcode.tex
	cat define-non-randomised-cycle | wc -l > t
	@echo "\\linesofcode"=`cat t` > corelinesofcode.tex
	cat java/*.java | ./uncom | wc -l > t
	@echo "\\linesofcode"=`cat t` > allLinesofcode.tex
	@rm -f t
	cc wine.c -o wine 
	./wine > winelist.tex 
	tail -n 1 winelist.tex > lastwine.tex
	sed -f sedcommands euler.c-tagged.txt > demo-tagged-euler.txt
	for i in caption-*.tex; do (head -c 30 $$i; echo "\ellipsis "; tail -c 30 $$i) > shorter-$$i; done
	pdflatex relit.tex # generate .aux and .idx files
	bibtex relit.aux
	makeindex relit.idx
	pdflatex relit.tex # now insert bibliography and index
	makeindex relit.idx # second run to get page numbers right
	pdflatex relit.tex # third run to sort out citation references to bibliography

relit: relit.c
	cc relit.c -o relit
	
uncom: uncom.c
	cc uncom.c -o uncom
	./uncom < java/DirectedEulerianCycle.java | wc -l > t
	@echo "\\linesofcode"=`cat t` > sedgewickeslinesofcode.tex
	@rm -f t
	
test: euler randomised-euler wine
	cc euler.c -o euler
	cc randomised-euler.c -o randomised-euler
	./euler
	./randomised-euler
	./wine

man: relit.1
	man ./relit.1 
	
zip:
	@echo To make zip files try:
	@echo "-  " make scp.zip  --- zips just the files needed by publishers to typeset the paper
	@echo "-  " make basic.zip    --- smallest package - zips all files, including documentation, source code, etc, ready for a make
	@echo "-  " make full.zip   --- zips all files, after a full make
	@echo "-  " make sw.zip   --- zips only the software, source and documentation, relit.c, draw-figures.nb, relit.1, etc
	
scp.zip: all
	@echo Zip for everything needed to upload to the journal Science of Computer Programming, everything ready to typeset
	zip scp.zip TeX-mode-demo.tex allLinesofcode.tex corelinesofcode.tex demo-tagged-euler.txt diff.txt java-output.tex lastwine.tex linesofcode.tex linesofrelit.tex nameDemo.c relit.aux relit.bbl relit.bib relit.idx relit.ind relit.spl relit.tex sedgewickeslinesofcode.tex shorter-caption-1.tex shorter-caption-2.tex shorter-caption-3.tex shorter-caption-4.tex shorter-caption-5.tex shorter-caption-6.tex winelist.tex Makefile figures

basic.zip: 
	@echo smallest self-contained zip file of everything, but needs a full make after unzipping
	zip basic.zip relit.tex Makefile relit.c relit.1 relit.bib draw-figures.nb figures README.md Highlights.txt java uncom.c unit-tests.txt 
	
full.zip: all
	@echo zip file of everything after a full make
	-rm -f full.zip
	zip full.zip *
	
sw.zip:
	@echo zip of just the software and documentation
	zip sw.zip Makefile draw-figures.nb relit.c relit.1 uncom.c unit-tests.txt
	
cleanlatex: # get rid of everything that Latex does NOT depend on -- use this you want to run latex only
	-rm -f define-non-randomised-cycle euler euler.c relit.dvi relit.log relit.spl hello.c randomised-euler randomised-euler.c relit-def.tex sedcommands TeX-mode-demo.tex uncom wine wine.c *-tagged.txt relit.zip

clean: cleanlatex # leaves just the sources, zips and the pdf file; after a clean, you'll have to do a make all
	-rm -f allLinesofcode.tex corelinesofcode.tex define-non-randomised-cycle demo-tagged-euler.txt euler euler.c relit.aux relit.blg relit.dvi relit.idx relit.ilg relit.log relit.spl relit.synctex.gz lastwine.tex linesofcode.tex linesofrelit.tex nameDemo.c randomised-euler randomised-euler.c relit relit-def.tex sedcommands sedgewickeslinesofcode.tex t TeX-mode-demo.tex uncom wine wine.c winelist.tex caption-*.tex shorter-caption-*.tex diff.txt relit.bbl relit.ind java-output.tex ForLoop.java ForLoop.class
	-rm -f *-tagged.txt
