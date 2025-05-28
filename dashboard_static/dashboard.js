// Global state
let socket;
let replayFrames = [];
let validationFrames = [];
let mismatches = [];
let currentFrameIdx = 0;
let isMonitoring = false;
let isLive = true;
let healthChart = null;
let positionChart = null;
let maxPlottedFrames = 1000; // Maximum number of frames to show on charts

// Stage bounds for positions
const stageBounds = {
    x_min: 0,
    x_max: 400,
    y_min: 0,
    y_max: 100
};

// Initialize Socket.IO connection
function initSocket() {
    socket = io();

    socket.on('connect', () => {
        console.log('Connected to server');
        updateMonitoringStatus();
    });

    socket.on('disconnect', () => {
        console.log('Disconnected from server');
        updateMonitoringStatus(false);
    });

    socket.on('new_replay_frames', (data) => {
        if (data.frames && data.frames.length > 0) {
            replayFrames.push(...data.frames);
            updateUI();

            // If we're in live mode, move to the latest frame
            if (isLive) {
                navigateToFrame(replayFrames.length - 1);
            }
        }
    });

    socket.on('new_validation_frames', (data) => {
        if (data.frames && data.frames.length > 0) {
            validationFrames.push(...data.frames);
            updateUI();
        }
    });

    socket.on('new_mismatches', (data) => {
        if (data.mismatches && data.mismatches.length > 0) {
            mismatches.push(...data.mismatches);
            updateMismatchesList();
            updateMismatchMarkers();
            
            // Show notification
            showNotification(`${data.mismatches.length} new mismatches detected`);
        }
    });

    socket.on('frame_data', (data) => {
        if (data.error) {
            console.error('Frame data error:', data.error);
            return;
        }
        displayFrame(data.replay_frame, data.validation_frame, data.has_validation);
    });
}

// Fetch initial data from API
async function fetchInitialData() {
    try {
        const statusResponse = await fetch('/api/status');
        const statusData = await statusResponse.json();
        
        isMonitoring = statusData.is_monitoring;
        updateMonitoringStatus(isMonitoring);
        
        document.getElementById('replayFrameCount').textContent = statusData.replay_frames_count;
        document.getElementById('mismatchCountBadge').textContent = statusData.mismatches_count;
        
        if (statusData.replay_frames_count > 0) {
            const framesResponse = await fetch(`/api/frames?start=0&end=${statusData.replay_frames_count}`);
            const framesData = await framesResponse.json();
            
            replayFrames = framesData.replay_frames || [];
            validationFrames = framesData.validation_frames || [];
            
            const mismatchesResponse = await fetch('/api/mismatches');
            const mismatchesData = await mismatchesResponse.json();
            
            mismatches = mismatchesData.mismatches || [];
            
            updateUI();
            
            // Navigate to the first frame or first mismatch if any
            if (mismatches.length > 0) {
                navigateToFrame(mismatches[0].index);
            } else if (replayFrames.length > 0) {
                navigateToFrame(0);
            }
        }
    } catch (error) {
        console.error('Error fetching initial data:', error);
    }
}

// Update UI based on current state
function updateUI() {
    updateFrameCount();
    updateSlider();
    updateCharts();
    updateMismatchesList();
    updateMismatchMarkers();
}

// Update frame count display
function updateFrameCount() {
    document.getElementById('replayFrameCount').textContent = replayFrames.length;
    document.getElementById('mismatchCountBadge').textContent = mismatches.length;
}

// Update slider range based on available frames
function updateSlider() {
    const slider = document.getElementById('frameSlider');
    slider.max = Math.max(0, replayFrames.length - 1);
    slider.value = currentFrameIdx;
}

// Update monitoring status indicators
function updateMonitoringStatus(status = null) {
    if (status !== null) {
        isMonitoring = status;
    }
    
    const statusIndicator = document.getElementById('monitoringStatus');
    const statusLabel = document.getElementById('monitoringStatusLabel');
    const toggleBtn = document.getElementById('toggleMonitoring');
    const statusIcon = document.getElementById('monitoringStatusIcon');
    const statusText = document.getElementById('monitoringStatusText');
    
    if (isMonitoring) {
        statusIndicator.className = 'status-indicator status-green';
        statusLabel.textContent = 'Monitoring';
        statusIcon.innerHTML = '<i class="bi bi-pause-circle"></i>';
        statusText.textContent = 'Stop Monitoring';
    } else {
        statusIndicator.className = 'status-indicator status-red';
        statusLabel.textContent = 'Not Monitoring';
        statusIcon.innerHTML = '<i class="bi bi-play-circle"></i>';
        statusText.textContent = 'Start Monitoring';
    }
}

// Toggle monitoring state
async function toggleMonitoring() {
    try {
        if (isMonitoring) {
            const response = await fetch('/api/stop_monitoring', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                }
            });
            const data = await response.json();
            isMonitoring = false;
        } else {
            const watchDir = document.getElementById('watchDir').value;
            const response = await fetch('/api/start_monitoring', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({
                    directory: watchDir || null
                })
            });
            const data = await response.json();
            isMonitoring = true;
        }
        updateMonitoringStatus();
    } catch (error) {
        console.error('Error toggling monitoring:', error);
    }
}

// Set active replay and validation files
async function setFiles() {
    const replayFile = document.getElementById('replayFile').value;
    const validationFile = document.getElementById('validationFile').value;
    const startMonitoring = document.getElementById('startMonitoring').checked;
    
    if (!replayFile && !validationFile) {
        alert('Please provide at least one file path');
        return;
    }
    
    try {
        const response = await fetch('/api/set_files', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                replay_file: replayFile || null,
                validation_file: validationFile || null
            })
        });
        
        const data = await response.json();
        
        if (data.error) {
            alert(`Error: ${data.error}`);
            return;
        }
        
        // Reset state
        replayFrames = [];
        validationFrames = [];
        mismatches = [];
        currentFrameIdx = 0;
        
        // Close modal
        const modal = bootstrap.Modal.getInstance(document.getElementById('filesModal'));
        modal.hide();
        
        // Fetch new data
        await fetchInitialData();
        
        // Start monitoring if requested
        if (startMonitoring && !isMonitoring) {
            await toggleMonitoring();
        }
    } catch (error) {
        console.error('Error setting files:', error);
        alert('Failed to set files. Check console for details.');
    }
}

// Navigate to a specific frame
function navigateToFrame(frameIdx) {
    if (frameIdx < 0 || frameIdx >= replayFrames.length) {
        return;
    }
    
    currentFrameIdx = frameIdx;
    document.getElementById('frameSlider').value = frameIdx;
    document.getElementById('currentFrameDisplay').textContent = `Frame: ${replayFrames[frameIdx].frame || frameIdx}`;
    
    // Update timeline marker
    const timeline = document.getElementById('frameTimeline');
    const marker = document.getElementById('timelineMarker');
    const percentage = (frameIdx / Math.max(1, replayFrames.length - 1)) * 100;
    marker.style.left = `${percentage}%`;
    
    // Request frame data from server
    socket.emit('request_frame', { frame_idx: frameIdx });
    
    // Update charts to highlight current frame
    updateChartMarkers(frameIdx);
    
    // Turn off live mode if navigating manually
    isLive = false;
}

// Display frame data
function displayFrame(replayFrame, validationFrame, hasValidation) {
    if (!replayFrame) return;
    
    // Update stage view (positions)
    const player1 = document.getElementById('player1');
    const player2 = document.getElementById('player2');
    const stageWidth = 400; // Match with CSS
    
    const p1X = replayFrame.p1_x || 0;
    const p2X = replayFrame.p2_x || 0;
    
    player1.style.left = `${p1X}px`;
    player2.style.left = `${p2X}px`;
    
    // Update health bars
    const p1Health = replayFrame.p1_health || 0;
    const p2Health = replayFrame.p2_health || 0;
    
    const p1HealthBar = document.getElementById('p1HealthBar');
    const p2HealthBar = document.getElementById('p2HealthBar');
    const p1HealthValue = document.getElementById('p1HealthValue');
    const p2HealthValue = document.getElementById('p2HealthValue');
    
    p1HealthBar.style.width = `${p1Health * 100}%`;
    p2HealthBar.style.width = `${p2Health * 100}%`;
    p1HealthValue.textContent = `${(p1Health * 100).toFixed(0)}%`;
    p2HealthValue.textContent = `${(p2Health * 100).toFixed(0)}%`;
    
    // Update inputs display
    const inputs = replayFrame.inputs || 0;
    
    // Assuming inputs is a bitmask, decode them
    const inputMap = {
        'up': 0,
        'down': 1,
        'left': 2,
        'right': 3,
        'b1': 4,
        'b2': 5,
        'b3': 6,
        'b4': 7,
        'b5': 8,
        'b6': 9,
        'start': 10,
        'coin': 11
    };
    
    // Reset all buttons
    for (const [inputName, bit] of Object.entries(inputMap)) {
        const button = document.getElementById(`input-${inputName}`);
        button.classList.remove('active');
    }
    
    // Set active buttons
    for (const [inputName, bit] of Object.entries(inputMap)) {
        if (inputs & (1 << bit)) {
            const button = document.getElementById(`input-${inputName}`);
            button.classList.add('active');
        }
    }
    
    // Update frame hash
    const frameHashDiv = document.getElementById('frameHash');
    const hashCompareDiv = document.getElementById('hashCompareResult');
    const frameHashDisplay = document.getElementById('frameHashDisplay');
    
    frameHashDiv.textContent = replayFrame.state_hash || 'N/A';
    
    if (hasValidation && validationFrame) {
        const validationHash = validationFrame.state_hash || 'N/A';
        
        if (replayFrame.state_hash !== validationHash) {
            hashCompareDiv.innerHTML = `
                <div class="alert alert-danger">
                    <strong>Hash Mismatch!</strong><br>
                    Validation: ${validationHash}
                </div>
            `;
            frameHashDisplay.classList.add('mismatch-highlight');
        } else {
            hashCompareDiv.innerHTML = `<div class="alert alert-success">Hashes Match ✓</div>`;
            frameHashDisplay.classList.remove('mismatch-highlight');
        }
    } else {
        hashCompareDiv.innerHTML = '';
        frameHashDisplay.classList.remove('mismatch-highlight');
    }
    
    // Update variables table
    const variablesTable = document.getElementById('variablesTable');
    variablesTable.innerHTML = '';
    
    const variablesToShow = [
        { key: 'p1_health', label: 'P1 Health' },
        { key: 'p2_health', label: 'P2 Health' },
        { key: 'p1_x', label: 'P1 X Position' },
        { key: 'p2_x', label: 'P2 X Position' },
        { key: 'rng_seed', label: 'RNG Seed' },
    ];
    
    for (const variable of variablesToShow) {
        const row = document.createElement('tr');
        
        const replayValue = replayFrame[variable.key];
        const validationValue = hasValidation && validationFrame ? validationFrame[variable.key] : null;
        const hasMismatch = hasValidation && validationFrame && replayValue !== validationValue;
        
        if (hasMismatch) {
            row.classList.add('mismatch-highlight');
        }
        
        row.innerHTML = `
            <td>${variable.label}</td>
            <td>${replayValue !== undefined ? replayValue : 'N/A'}</td>
            ${hasValidation ? `<td>${validationValue !== undefined ? validationValue : 'N/A'}</td>` : ''}
        `;
        
        variablesTable.appendChild(row);
    }
}

// Initialize and update charts
function initializeCharts() {
    const healthCtx = document.getElementById('healthChart').getContext('2d');
    const positionCtx = document.getElementById('positionChart').getContext('2d');
    
    // Health chart
    healthChart = new Chart(healthCtx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [
                {
                    label: 'P1 Health',
                    data: [],
                    borderColor: 'blue',
                    backgroundColor: 'rgba(0, 0, 255, 0.1)',
                    borderWidth: 1,
                    tension: 0.1
                },
                {
                    label: 'P2 Health',
                    data: [],
                    borderColor: 'red',
                    backgroundColor: 'rgba(255, 0, 0, 0.1)',
                    borderWidth: 1,
                    tension: 0.1
                }
            ]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            scales: {
                y: {
                    beginAtZero: true,
                    max: 1
                }
            },
            animation: false,
            plugins: {
                annotation: {
                    annotations: {
                        currentFrame: {
                            type: 'line',
                            xMin: 0,
                            xMax: 0,
                            borderColor: 'green',
                            borderWidth: 2
                        }
                    }
                }
            }
        }
    });
    
    // Position chart
    positionChart = new Chart(positionCtx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [
                {
                    label: 'P1 X Position',
                    data: [],
                    borderColor: 'blue',
                    backgroundColor: 'rgba(0, 0, 255, 0.1)',
                    borderWidth: 1,
                    tension: 0.1
                },
                {
                    label: 'P2 X Position',
                    data: [],
                    borderColor: 'red',
                    backgroundColor: 'rgba(255, 0, 0, 0.1)',
                    borderWidth: 1,
                    tension: 0.1
                }
            ]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            scales: {
                y: {
                    min: stageBounds.x_min,
                    max: stageBounds.x_max
                }
            },
            animation: false,
            plugins: {
                annotation: {
                    annotations: {
                        currentFrame: {
                            type: 'line',
                            xMin: 0,
                            xMax: 0,
                            borderColor: 'green',
                            borderWidth: 2
                        }
                    }
                }
            }
        }
    });
}

// Update charts with current data
function updateCharts() {
    if (!healthChart || !positionChart || replayFrames.length === 0) return;
    
    // Prepare data for charts
    let startIdx = Math.max(0, replayFrames.length - maxPlottedFrames);
    let frames = replayFrames.slice(startIdx);
    
    const frameNumbers = frames.map(frame => frame.frame || 0);
    const p1Health = frames.map(frame => frame.p1_health || 0);
    const p2Health = frames.map(frame => frame.p2_health || 0);
    const p1X = frames.map(frame => frame.p1_x || 0);
    const p2X = frames.map(frame => frame.p2_x || 0);
    
    // Update health chart
    healthChart.data.labels = frameNumbers;
    healthChart.data.datasets[0].data = p1Health;
    healthChart.data.datasets[1].data = p2Health;
    
    // Update position chart
    positionChart.data.labels = frameNumbers;
    positionChart.data.datasets[0].data = p1X;
    positionChart.data.datasets[1].data = p2X;
    
    // Update both charts
    healthChart.update();
    positionChart.update();
}

// Update chart vertical line marker for current frame
function updateChartMarkers(frameIdx) {
    if (!healthChart || !positionChart || replayFrames.length === 0) return;
    
    // Get actual frame number
    const frameNum = replayFrames[frameIdx]?.frame || 0;
    
    // We need to find the chart data index corresponding to this frame
    const healthChartIndex = healthChart.data.labels.indexOf(frameNum);
    const positionChartIndex = positionChart.data.labels.indexOf(frameNum);
    
    if (healthChartIndex >= 0) {
        const xPos = healthChart.scales.x.getPixelForValue(healthChartIndex);
        // Update annotation (if chart.js annotation plugin available)
        // If not available, use a custom solution
    }
    
    if (positionChartIndex >= 0) {
        const xPos = positionChart.scales.x.getPixelForValue(positionChartIndex);
        // Same for position chart
    }
}

// Update the mismatches list display
function updateMismatchesList() {
    const mismatchList = document.getElementById('mismatchList');
    
    if (mismatches.length === 0) {
        mismatchList.innerHTML = `
            <div class="text-center text-muted py-5">
                No mismatches detected
            </div>
        `;
        return;
    }
    
    mismatchList.innerHTML = '';
    
    for (const mismatch of mismatches) {
        const card = document.createElement('div');
        card.className = 'card frame-card mismatch';
        card.setAttribute('data-frame-idx', mismatch.index);
        
        let mismatchTypesBadges = '';
        for (const type of mismatch.types) {
            mismatchTypesBadges += `<span class="badge bg-danger me-1">${type}</span>`;
        }
        
        card.innerHTML = `
            <div class="card-header d-flex justify-content-between align-items-center">
                <div>Frame ${mismatch.frame}</div>
                <button class="btn btn-sm btn-outline-secondary goto-frame-btn">Go</button>
            </div>
            <div class="card-body">
                <div class="mb-2">${mismatchTypesBadges}</div>
                <div class="small">
                    Index: ${mismatch.index}<br>
                    Types: ${mismatch.types.join(', ')}
                </div>
            </div>
        `;
        
        card.querySelector('.goto-frame-btn').addEventListener('click', () => {
            navigateToFrame(mismatch.index);
        });
        
        mismatchList.appendChild(card);
    }
}

// Update mismatch markers on the timeline
function updateMismatchMarkers() {
    const timeline = document.getElementById('frameTimeline');
    
    // Remove existing mismatch markers
    const existingMarkers = timeline.querySelectorAll('.mismatch-marker');
    existingMarkers.forEach(marker => marker.remove());
    
    // Add markers for each mismatch
    for (const mismatch of mismatches) {
        const percentage = (mismatch.index / Math.max(1, replayFrames.length - 1)) * 100;
        
        const marker = document.createElement('div');
        marker.className = 'mismatch-marker';
        marker.style.left = `${percentage}%`;
        marker.setAttribute('data-frame-idx', mismatch.index);
        marker.setAttribute('title', `Mismatch at frame ${mismatch.frame}`);
        
        marker.addEventListener('click', () => {
            navigateToFrame(mismatch.index);
        });
        
        timeline.appendChild(marker);
    }
}

// Clear all mismatches
function clearMismatches() {
    mismatches = [];
    updateMismatchesList();
    updateMismatchMarkers();
    document.getElementById('mismatchCountBadge').textContent = '0';
}

// Navigate to next mismatch
function nextMismatch() {
    if (mismatches.length === 0) return;
    
    // Find the next mismatch after current frame
    const nextMismatch = mismatches.find(m => m.index > currentFrameIdx);
    
    if (nextMismatch) {
        navigateToFrame(nextMismatch.index);
    } else if (mismatches.length > 0) {
        // Wrap around to first mismatch
        navigateToFrame(mismatches[0].index);
    }
}

// Navigate to previous mismatch
function prevMismatch() {
    if (mismatches.length === 0) return;
    
    // Find the previous mismatch before current frame
    const prevMismatches = mismatches.filter(m => m.index < currentFrameIdx);
    
    if (prevMismatches.length > 0) {
        navigateToFrame(prevMismatches[prevMismatches.length - 1].index);
    } else if (mismatches.length > 0) {
        // Wrap around to last mismatch
        navigateToFrame(mismatches[mismatches.length - 1].index);
    }
}

// Show a notification
function showNotification(message) {
    // For now, just log to console, but could use a toaster library
    console.log('Notification:', message);
}

// Setup event listeners
function setupEventListeners() {
    // Toggle monitoring
    document.getElementById('toggleMonitoring').addEventListener('click', (e) => {
        e.preventDefault();
        toggleMonitoring();
    });
    
    // Set files button
    document.getElementById('setFilesBtn').addEventListener('click', setFiles);
    
    // Frame slider
    document.getElementById('frameSlider').addEventListener('input', (e) => {
        navigateToFrame(parseInt(e.target.value));
    });
    
    // Frame navigation buttons
    document.getElementById('prevFrameBtn').addEventListener('click', () => {
        navigateToFrame(currentFrameIdx - 1);
    });
    
    document.getElementById('nextFrameBtn').addEventListener('click', () => {
        navigateToFrame(currentFrameIdx + 1);
    });
    
    document.getElementById('prevMismatchBtn').addEventListener('click', prevMismatch);
    document.getElementById('nextMismatchBtn').addEventListener('click', nextMismatch);
    
    document.getElementById('liveBtn').addEventListener('click', () => {
        isLive = true;
        if (replayFrames.length > 0) {
            navigateToFrame(replayFrames.length - 1);
        }
    });
    
    // Mismatches refresh/clear buttons
    document.getElementById('refreshMismatchesBtn').addEventListener('click', async () => {
        try {
            const response = await fetch('/api/mismatches');
            const data = await response.json();
            mismatches = data.mismatches || [];
            updateMismatchesList();
            updateMismatchMarkers();
        } catch (error) {
            console.error('Error refreshing mismatches:', error);
        }
    });
    
    document.getElementById('clearMismatchesBtn').addEventListener('click', clearMismatches);
    
    // Keyboard shortcuts
    document.addEventListener('keydown', (e) => {
        // Arrow left/right for frame navigation
        if (e.key === 'ArrowLeft') {
            navigateToFrame(currentFrameIdx - 1);
        } else if (e.key === 'ArrowRight') {
            navigateToFrame(currentFrameIdx + 1);
        } 
        // Page up/down for faster navigation
        else if (e.key === 'PageUp') {
            navigateToFrame(currentFrameIdx - 10);
        } else if (e.key === 'PageDown') {
            navigateToFrame(currentFrameIdx + 10);
        }
        // Home/End for first/last frame
        else if (e.key === 'Home') {
            navigateToFrame(0);
        } else if (e.key === 'End') {
            navigateToFrame(replayFrames.length - 1);
        }
        // M for next mismatch
        else if (e.key === 'm') {
            nextMismatch();
        }
        // N for previous mismatch
        else if (e.key === 'n') {
            prevMismatch();
        }
        // L for live mode
        else if (e.key === 'l') {
            isLive = true;
            if (replayFrames.length > 0) {
                navigateToFrame(replayFrames.length - 1);
            }
        }
    });
}

// Init function
function init() {
    // Initialize UI components
    initSocket();
    initializeCharts();
    setupEventListeners();
    fetchInitialData();
}

// Start when DOM is ready
document.addEventListener('DOMContentLoaded', init); 