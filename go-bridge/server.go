package main

import (
    "bytes"
    "fmt"
    "io/ioutil"
    "log"
    "net/http"
    "os"
    "os/exec"
    "path/filepath"
    "time"
)

// The text-based heap profile endpoint from your C++ app
const cppAppHeapURL = "http://cpp-app:8080/debug/pprof/heap"

// Path to your compiled C++ binary for symbolization
const cxxBinaryPath = "/build-output/heap_profile_server"

// Where we store temporary .txt & .pprof files
var tempDir = "/tmp"

// Pyroscope server where we'll upload
const pyroscopeURL = "http://host.docker.internal:4040"

func main() {
    // 1) Start a background goroutine that profiles every 30s
    go startContinuousProfiling(30 * time.Second)

    // 2) Optionally, expose an HTTP endpoint if you want to trigger or debug
    http.HandleFunc("/profile", handleManualProfile)
    log.Println("Bridging server listening on :8081")
    log.Fatal(http.ListenAndServe(":8081", nil))
}

// startContinuousProfiling: runs fetchAndUploadProfile() every 'interval'
func startContinuousProfiling(interval time.Duration) {
    ticker := time.NewTicker(interval)
    defer ticker.Stop()

    for {
        <-ticker.C
        if err := fetchAndUploadProfile(); err != nil {
            log.Printf("Error in continuous profiling: %v", err)
        } else {
            log.Println("Continuous profiling uploaded successfully")
        }
    }
}

// handleManualProfile: optional HTTP endpoint to do a one-off profile
func handleManualProfile(w http.ResponseWriter, r *http.Request) {
    if err := fetchAndUploadProfile(); err != nil {
        log.Printf("Error fetching/uploading profile: %v", err)
        http.Error(w, "failed to profile", http.StatusInternalServerError)
        return
    }
    fmt.Fprintln(w, "Profile fetched and uploaded successfully!")
}

// fetchAndUploadProfile does the 3-step process:
//  1) Grab text from the C++ app
//  2) Convert to .pprof
//  3) Upload to Pyroscope with profilecli
func fetchAndUploadProfile() error {
    // 1. Fetch text-based heap profile from the C++ app
    resp, err := http.Get(cppAppHeapURL)
    if err != nil {
        return fmt.Errorf("http get error: %w", err)
    }
    defer resp.Body.Close()

    if resp.StatusCode != http.StatusOK {
        return fmt.Errorf("C++ app responded with status %d", resp.StatusCode)
    }
    textData, err := ioutil.ReadAll(resp.Body)
    if err != nil {
        return fmt.Errorf("reading body error: %w", err)
    }

    // 2. Write text data & convert via pprof --proto
    timeStamp := time.Now().UnixNano()
    textFile := filepath.Join(tempDir, fmt.Sprintf("heap-%d.txt", timeStamp))
    if err := ioutil.WriteFile(textFile, textData, 0644); err != nil {
        return fmt.Errorf("write text file error: %w", err)
    }
    defer os.Remove(textFile)

    protoFile := filepath.Join(tempDir, fmt.Sprintf("heap-%d.pprof", timeStamp))
    defer os.Remove(protoFile)

    // pprof --proto cxxBinaryPath textFile > protoFile
    convertCmd := exec.Command("pprof", "--proto", cxxBinaryPath, textFile)
    outFile, err := os.Create(protoFile)
    if err != nil {
        return fmt.Errorf("create proto file error: %w", err)
    }
    defer outFile.Close()
    convertCmd.Stdout = outFile
    convertCmd.Stderr = os.Stderr
    if err := convertCmd.Run(); err != nil {
        return fmt.Errorf("pprof conversion error: %w", err)
    }

    // 3. Upload .pprof to Pyroscope with profilecli
    uploadCmd := exec.Command("profilecli",
        "upload",
        "--url="+pyroscopeURL,
        "--extra-labels=service_name=cpp-heap", // e.g. add your own labels
        protoFile,
    )
    var uploadOut bytes.Buffer
    uploadCmd.Stdout = &uploadOut
    uploadCmd.Stderr = os.Stderr
    if err := uploadCmd.Run(); err != nil {
        return fmt.Errorf("profilecli upload error: %w", err)
    }
    log.Printf("profilecli upload success: %s", uploadOut.String())
    return nil
}
