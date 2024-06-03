
gen-sbe:
	java -Dsbe.output.dir=. -Dsbe.target.language=Rust -jar ../simple-binary-encoding/sbe-all/build/libs/sbe-all-1.32.0-SNAPSHOT.jar sbe.xml
