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
        globe: false,
        skybox: null
    });

    var tileset = new Cesium.Cesium3DTileset({
        url: 'http://localhost:8081/tileset.json'
    });
    tileset.maximumScreenSpaceError = 1000;
    // tileset.dynamicScreenSpaceError = true;

    tileset.readyPromise.then(function(tileset) {
        viewer.zoomTo(tileset, new Cesium.HeadingPitchRange(0.5, -0.2, tileset.boundingSphere.radius * 4.0));
        // tileset.debugShowBoundingVolume = true;
    }).otherwise(function(error) {
        console.log(error);
    });
    viewer.scene.primitives.add(tileset);
    viewer.scene.skyBox.destroy();
    viewer.scene.skyBox = undefined;
    viewer.scene.sun.destroy();
    viewer.scene.sun = undefined;
    viewer.scene.backgroundColor = Cesium.Color.WHITE.clone();
    // new Cesium.CesiumInspector(document.getElementById("inspector"),  viewer.scene)
    // viewer.extend(Cesium.viewerCesiumInspectorMixin);
    viewer.extend(Cesium.viewerCesium3DTilesInspectorMixin);
    // viewer.extend(Cesium.viewerDragDropMixin);
    // viewer.extend(Cesium.viewerPerformanceWatchdogMixin);

    var lastFeature = null;
    var handler = new Cesium.ScreenSpaceEventHandler(viewer.scene.canvas);
    handler.setInputAction(function(movement) {
        var feature = viewer.scene.pick(movement.endPosition);

        // unselectFeature(selectedFeature);

        if (feature instanceof Cesium.Cesium3DTileFeature) {
            // console.log(feature);
            // selectFeature(feature);
            if (lastFeature) {
                lastFeature.color = Cesium.Color.WHITE;
                // lastFeature.show = true;
            }
            // feature.primitive.debugShowBoundingVolume = true;
            // feature.show =false;
            feature.color = Cesium.Color.RED;
            lastFeature = feature;
        }
    }, Cesium.ScreenSpaceEventType.MOUSE_MOVE);

}
if (typeof Cesium !== 'undefined') {
    startup(Cesium);
} else if (typeof require === 'function') {
    require(['Cesium'], startup);
}
