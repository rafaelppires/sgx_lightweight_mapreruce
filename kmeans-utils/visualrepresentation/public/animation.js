var isPlaying = false;
var manualStepIds = ["#steps", "#previous", "#next"];
var autoStepIds = ["#stop", "#start"];

function setDisabledState(state) {
    $("#stop").prop("disabled", !state);
    $("#play").prop("disabled", state);
    for (var i = 0; i < manualStepIds.length; ++i) {
        $(manualStepIds[i]).prop("disabled", state);
    }
}

function play() {
    setDisabledState(true);
    isPlaying = true;

    var id = setInterval(frame, 500);

    function frame() {        
        if (!isPlaying) {
            clearInterval(id);
        } else {
            loadStep(currentStep);
            goNext();
        }
    }
}

function stop() {
    setDisabledState(false);
    isPlaying = false;
}
