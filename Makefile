
grammar: Tmplang.g4
	java -jar /usr/local/lib/antlr-4.9.2-complete.jar -Dlanguage=Cpp -o src Tmplang.g4