# Jcp

## What is it?

Jcp is an open source cross language call framework based on FFI. It aims to provide a high-performance

framework of calling between different languages.

## Where to get it

Python binary installers for the latest released version are available at the [Python package index](https://pypi.org/project/pyjcp)

```bash
pip install pyjcp
```

Java Maven Dependency
```
<dependency>
    <groupId>alibaba</groupId>
    <artifactId>jcp</artifactId>
    <version>{version}</version>
</dependency>
```

## Dependencies

- [NumPy - Adds support for large, multi-dimensional arrays, matrices and high-level mathematical functions to operate on these arrays](https://www.numpy.org)

## Installation from sources

Prerequisites for building Jcp:

* Unix-like environment (we use Linux, Mac OS X)
* Git
* Maven (we recommend version 3.2.5 and require at least 3.1.1)
* Java 8 or 11 (Java 9 or 10 may work)
* Python >= 3.7 (we recommend version 3.7, 3.8, 3.9)

```
git clone https://github.com/apache/jcp.git
cd jcp
mvn clean install -DskipTests
pip install -r dev/dev-requirements.txt
python setup.py sdist
pip install dist/*.tar.gz
```

## Usage

```java
String path = ...;
PythonInterpreterConfig config = PythonInterpreterConfig
    .newBuilder()
    .setPythonExec("python3") // specify python exec
    .addPythonPaths(path) // add path to search path
    .build();

PythonInterpreter interpreter = new PythonInterpreter(config);

// set & get
interpreter.set("a", 12345);
interpreter.get("a"); // Object
interpreter.get("a", int.class); // int

// exec & eval
interpreter.exec("print(a)");

// invoke args
interpreter.exec("import str_upper");
String result = interpreter.invoke("str_upper.upper", "abcd");
// Object invoke(String name, Object... args);
// Object invoke(String name, Object[] args, Map<String, Object> kwargs);


interpreter.close();
```

## Documentation
