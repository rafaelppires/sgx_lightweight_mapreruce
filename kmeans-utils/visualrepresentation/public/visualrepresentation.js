var steps = [], points = [];

var currentStep = 0, nrSteps = 0;

$.get('/data', function (serverData, status) {
    $("#stop").prop("disabled", true);      // disable stop button at first

    appendSteps(serverData.stepsDomain);    // prepare steps

    steps = serverData.steps;

    // prepare points
    _.forEach(serverData.points, function (point) {
        points.push(createPoint(point, -1)); // no center assigned yet
    });

    nrSteps = steps.length - 1;

    loadStep(currentStep);
});

// watch on the select html tag and do something when has changed
$("#steps").change(function () {
    // when the step selection was changed
    currentStep = $(this).val(); // update the current step

    loadStep(currentStep);
});

function appendSteps(data) {
    for (var i = 0; i < data.length; ++i) {
        var step = data[i];

        $("#steps").append(
            $("<option>", {
                text: step,
                value: step
            })
        );
    }
}

function createPoint(point, centerId) {
    point.cluster = centerId;
    point.isCenter = point.id !== undefined;

    return point;
}

function calculateEuclidianDistance(point, otherPoint) {
    var x = (point.x - otherPoint.x) * (point.x - otherPoint.x);
    var y = (point.y - otherPoint.y) * (point.y - otherPoint.y);

    return Math.sqrt(x + y);
}

function getClosestCenter(centers, point) {
    // find the closest center to assign the point
    var center = _.minBy(centers, function (center) {
        return calculateEuclidianDistance(center, point);
    });

    return center.id;
}

function loadStep(givenStep) {
    var step = _.chain(steps)
        .filter(function (step) {
            return step.step == givenStep;
        })
        .head()
        .value();

    loadRepresentation(step);
}

function preparePoints(step) {
    // remove old centers
    _.remove(points, function (point) {
        return point.id !== undefined;
    });

    // assign points to clusters
    _.forEach(points, function (point) {
        point.cluster = getClosestCenter(step.centers, point);
    });

    // insert the center
    _.forEach(step.centers, function (center) {
        points.push(createPoint(center, center.id));
    });

    points = _.orderBy(points, 'cluster', ['asc']);
}

function loadRepresentation(step) {
    preparePoints(step);

    $("#result").empty(); // remove the old representation

    var margin = { top: 20, right: 20, bottom: 30, left: 40 },
        width = $("#result").width() - margin.left - margin.right,
        height = 700 - margin.top - margin.bottom;

    var x = d3.scale.linear().range([0, width]);

    var y = d3.scale.linear().range([height, 0]);

    var color = d3.scale.category20();

    var xAxis = d3.svg.axis().scale(x).orient("bottom").ticks(20);

    var yAxis = d3.svg.axis().scale(y).orient("left").ticks(20);

    var svg = d3.select("#result").append("svg")
        .attr("width", width + margin.left + margin.right)
        .attr("height", height + margin.top + margin.bottom)
        .append("g")
        .attr("transform", "translate(" + margin.left + "," + margin.top + ")");

    x.domain(d3.extent(points, function (d) { return d.x; })).nice();
    y.domain(d3.extent(points, function (d) { return d.y; })).nice();

    svg.append("g")
        .attr("class", "x axis")
        .attr("transform", "translate(0, " + height + ")")
        .call(xAxis)
        .append("text")
        .attr("class", "label")
        .attr("x", width)
        .attr("y", -6)
        .style("text-anchor", "end")
        .text("x axis");

    svg.append("g")
        .attr("class", "y axis")
        .call(yAxis)
        .append("text")
        .attr("class", "label")
        .attr("transform", "rotate(-90)")
        .attr("y", 6)
        .attr("dy", ".71em")
        .style("text-anchor", "end")
        .text("y axis")

    svg.selectAll(".dot")
        .data(points)
        .enter()
        .append("circle")
        .attr("class", "dot")
        .attr("r", function (d) { return d.isCenter ? 8.5 : 3.5; })
        .attr("cx", function (d) { return x(d.x); })
        .attr("cy", function (d) { return y(d.y); })
        .style("fill", function (d) { return color(d.cluster); });

    var legend = svg.selectAll(".legend")
        .data(color.domain())
        .enter().append("g")
        .attr("class", "legend")
        .attr("transform", function (d, i) { return "translate(20, " + i * 20 + ")"; });

    legend.append("rect")
        .attr("x", width - 18)
        .attr("width", 18)
        .attr("height", 18)
        .style("fill", color);

    legend.append("text")
        .attr("x", width - 24)
        .attr("y", 9)
        .attr("dy", ".35em")
        .style("text-anchor", "end")
        .text(function (d) { return d; });
}

function goPrevious() {
    if (currentStep == 0) {
        currentStep = nrSteps;
    } else {
        currentStep--;
    }

    $("#steps").val(currentStep).change();
}

function goNext() {
    if (currentStep == nrSteps) {
        currentStep = 0;
    } else {
        currentStep++;
    }

    $("#steps").val(currentStep).change();
}