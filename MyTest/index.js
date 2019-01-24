var startup = function(Cesium) {
    // A simple demo of 3D Tiles feature picking with hover and select behavior
    // Building data courtesy of NYC OpenData portal: http://www1.nyc.gov/site/doitt/initiatives/3d-building.page
    var viewer = new Cesium.Viewer('cesiumContainer', {
        geocoder: false,
        fullscreenButton: false,
        sceneModePicker: false,
        animation: false,
        selectionIndicator: false,
        baseLayerPicker: false,
        homeButton: false,
        infoBox: false,
        timeline: false,
        navigationHelpButton: false,
        navigationInstructionsInitiallyVisible: false,
        creditContainer: 'dummy',
        globe: false
    });

    // Set the initial camera view to look at Manhattan
    // var initialPosition = Cesium.Cartesian3.fromDegrees(-74.01881302800248, 40.69114333714821, 753);
    // var initialOrientation = new Cesium.HeadingPitchRoll.fromDegrees(21.27879878293835, -21.34390550872461, 0.0716951918898415);
    // viewer.scene.camera.setView({
    //     destination: initialPosition,
    //     orientation: initialOrientation,
    //     endTransform: Cesium.Matrix4.IDENTITY
    // });

    // Load the NYC buildings tileset
    var tileset = new Cesium.Cesium3DTileset({
        url: 'http://localhost:8081/tileset.json'
    });
    tileset.readyPromise.then(function(tileset) {
        viewer.zoomTo(tileset, new Cesium.HeadingPitchRange(0.5, -0.2, tileset.boundingSphere.radius * 4.0));
    }).otherwise(function(error) {
        console.log(error);
    });
    viewer.scene.primitives.add(tileset);
    // new Cesium.CesiumInspector(document.getElementById("inspector"),  viewer.scene)
    // viewer.extend(Cesium.viewerCesiumInspectorMixin);
    viewer.extend(Cesium.viewerCesium3DTilesInspectorMixin);
    // viewer.extend(Cesium.viewerDragDropMixin);
    // viewer.extend(Cesium.viewerPerformanceWatchdogMixin);

}
if (typeof Cesium !== 'undefined') {
    startup(Cesium);
} else if (typeof require === 'function') {
    require(['Cesium'], startup);
}
