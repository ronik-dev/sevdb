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
