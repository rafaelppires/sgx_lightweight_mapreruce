# What to install?
* Install NodeJS
    - Example Ubuntu: sudo apt-get install -y nodejs
* Make sure you are in "visualrepresentation" folder from a terminal
* Run command "npm install"
    - Verify that node_modules folder is created
* Run command "node index.js"
* Open a browser and access url: http://localhost:8000/
* The application won't work if you don't have folder "results" with files "steps.json" and "points.json".
In order to get those, run lua implementation from "lua-kmeans" folder with command "lua main.lua".

# What happened?
* When the url "http://localhost:8000/" is accessed, a request to a node js server is made to "http://localhost:8000/data"
* First, the server loads files "points.json" and "steps.json" from folder "./results" and sends the data.
* The client prepares the data and loads the first step.