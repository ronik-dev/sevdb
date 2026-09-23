# sevdb
*Small Embedded Vector Database*

#### Goal
This is a personal project that aims to create a simple embedded vector database in C.
This does not intend do be a finished production ready product, as i am more intrested in learning how vector databeses work and improve my C programming language skills.
The main functionalities i aim to produce are:
- Uploading new vectors to the database
- Deleting vectors from the database
- Implement a retrival algorithm (starting from linear search, maybe later move to HNSW)
- Support mutiple vector sizes on request
- Serialize the db to a local file
- Deserialize the db from a local file

This project will not (at least in the short term)
- Provide authentication/authorization to the database or the stored data
- Provide encryption of any kind for the serialized data
- Provide a structured query language, functionalities will be accessible via the program api

#### Build Test and Run
The project relies on [Docker](https://www.docker.com/) to provide a consistent C17 build environment with [CMake](https://cmake.org/) and [Criterion](https://criterion.readthedocs.io/en/master/intro.html)* pre-installed. 
In the future i will provide a build without testing dependencies, but its out of scope for now.

Use the included `Makefile` to interact with the project:
- **Build the project:** `make build`
- **Run the test suite:** `make test`
- **Clean the environment:** `make clean`
- **Open a dev shell:** `make shell`
