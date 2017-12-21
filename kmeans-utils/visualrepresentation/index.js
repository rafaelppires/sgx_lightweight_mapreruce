var fs = require('fs');

var _ = require('lodash');

var express = require('express');
var app = express();

var path = require('path');

app.use(express.static('public'));

app.get('/data', function(req, res) {
    var result = {};

    fs.readFile('./results/points.json', 'utf-8', function (err, points) {
        result.points = JSON.parse(points)

        fs.readFile('./results/steps.json', 'utf-8', function(err, steps) {
            var steps = JSON.parse(steps);

            // order by step number and order the centers by id from each step
            result.steps = _.chain(steps)
                            .orderBy('step', ['asc'])
                            .map(function(step) {
                                return {
                                    step: step.step,
                                    centers: _.orderBy(step.centers, ['id'], ['asc']) 
                                };
                            })
                            .value();

            result.stepsDomain = _.chain(result.steps)
                                  .map(function(step) {
                                      return step.step;
                                  })
                                  .value();
                                  

            return res.json(result);
        });
    });
});

app.get('/', function(req, res) {
    res.sendFile(path.join(__dirname + '/index.html'));
});

app.listen(8000);