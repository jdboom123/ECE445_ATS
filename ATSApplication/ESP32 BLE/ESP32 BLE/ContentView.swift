//
//  ContentView.swift
//  ESP32 BLE
//
//  Created by Artsemi Ryzhankou on 19/01/2024.
//

import SwiftUI
import Combine
import Charts


struct ContentView: View {

    @ObservedObject var bleManager = BLEManager()

    @State var isEsp32LedEnabled = false
    @State var sevenSegmentValue = 0
    @State var trafficLightColor = "RED"

    @State var isAutoSwitched = false
    @State private var timer: Timer.TimerPublisher = Timer.publish(every: 1, on: .main, in: .common)
    @State private var cancellable: AnyCancellable?
    
    // Added variables
    let exercises = ["Bench Press", "Weighted Squat"]
    let exerciseBLE = ["BenchPress", "WeightedSquat"]
    @State var choiceMade = "Choose Exercise"
    @State var selectedExercise = ""
   
    @State var isPopUpActive = false
    
    @State private var thresholdVelocity: String = ""
    @State var testVelocity: String = ""
    @State var validVelocity: Bool = false
    @FocusState private var isActive: Bool
    let formatter = NumberFormatter()
    
    @State var isGraphActive = false
    @State var thresholdSetting = false

    var body: some View {
        NavigationStack {
            ZStack{
            VStack(alignment: .leading) {
                 if bleManager.peripherals.isEmpty {
                     ProgressView("Searching for Athletic Tracking Sensors")
                         .progressViewStyle(.circular)
                 } else if bleManager.isConnected {
                    List {
                        Section("Exercise Selection") {
                            exerciseTable
                        }
                        Section("Set Target Velocity") {
                            velocityTextBox
                            validVelocityText
                        }
                        Section("Threshold Settings"){
                            thresholdControl
                        }
                        Button("Send Data") {
                            if validVelocity && selectedExercise != ""{
                                if !bleManager.recordData {
                                    bleManager.sendTextValue("\(selectedExercise) \(thresholdVelocity)")
                                    bleManager.graphView = false
                                }
                            }
                            else{
                                isPopUpActive = true
                                
                            }
                        }
                        
                        
                        NavigationLink("Present Velocity Graph") {
                            if bleManager.graphView{
                                VelocityGraphView(thresholdVelocityGraph: Double(thresholdVelocity) ?? 0,unidentifiableData: bleManager.velocityPointsArray.dataPoints)
                            }
                            else{
                                PopUpError(popupActive: $isPopUpActive, title: "No Data to Present", message: "Record data using the Athletic Tracking Sensor to create a graph.", buttonTitle: "Okay", action: {})
                            }
                        }
                        
                        NavigationLink("Present Angle Graph") {
                            if bleManager.graphView{
                                AngleGraphView(unidentifiableData: bleManager.anglePointsArray.dataPoints)
                            }
                            else{
                                PopUpError(popupActive: $isPopUpActive, title: "No Data to Present", message: "Record data using the Athletic Tracking Sensor to create a graph.", buttonTitle: "Okay", action: {})
                            }
                        }
                        
                        
                        Button("Disconnect") {
                            bleManager.graphView = false
                            bleManager.disconnectFromPeripheral()
                            choiceMade = "Choose Exercise"
                            thresholdVelocity = ""
                            isPopUpActive = false
                            bleManager.velocityPointsArray.dataPoints.removeAll()
                            bleManager.anglePointsArray.dataPoints.removeAll()
                        }
                        .foregroundStyle(.red)
                        .frame(maxWidth: .infinity, alignment: .center)
                        
                        //                        Button("Disconnect") {
                        //                            bleManager.disconnectFromPeripheral()
                        //                        }
                        //                        .foregroundStyle(.red)
                        //                        .frame(maxWidth: .infinity, alignment: .center)
                    }
                    .onAppear {
                        thresholdSetting = false
                        sevenSegmentValue = 0
                        trafficLightColor = "RED"
                        isAutoSwitched = false
                        cancellable?.cancel()
                    }

                } else {
                         List(bleManager.peripherals) { peripheral in
                             device(peripheral)
                         }
                         .refreshable {
                             bleManager.refreshDevices()
                         }
                     }
            }
            .navigationTitle("ATS Control")
            .toolbar(content: {
                ToolbarItemGroup(placement: .navigationBarTrailing) {
                    NavigationStack{
                        HStack{
                            NavigationLink(destination: InfoView()) {
                                Image(systemName: "info.circle")
                                    .resizable()
                                    .frame(width: 35, height: 35)
                                    .foregroundColor(.blue)
                                    .padding()
                            }
                        }
                        
                    }
                    .padding()
                }
            })
                
                
            if isPopUpActive{
                PopUpError(popupActive: $isPopUpActive, title: "Data is Invalid", message: "Please select a proper exercise and/or enter a proper number for the velocity", buttonTitle: "Okay", action: {})
                    .onDisappear()
            }
//            if isPopUpActive && bleManager.graphView == false{
//                PopUpError(popupActive: $isPopUpActive, title: "No Data to Present", message: "Record data using the Athletic Tracking Sensor to create a graph.", buttonTitle: "Okay", action: {})
//                }
            }
//            if isGraphActive{
//                VelocityGraphView(unidentifiableData: bleManager.dataPointsArray.dataPoints)
//            }
        }
        
    }
    
    
    // New Elements //
    
    var exerciseTable: some View {
        return Menu {
            ForEach(exercises, id: \.self) { exercise in
                Button(action: {
                    choiceMade = exercise
                    switch choiceMade{
                    case exercises[0]:
                        selectedExercise = exerciseBLE[0]
                    case exercises[1]:
                        selectedExercise = exerciseBLE[1]
                    default:
                        selectedExercise = ""
                    }
                }) {
                    Text(exercise)
                }
            }
        } label: {
            Label(
                title: { Text(choiceMade) },
                icon: { Image(systemName: "plus") }
                
            )
            .frame(maxWidth: .infinity, alignment: .center)
            .foregroundStyle(.primary)
            
        }
    }
    
    var velocityTextBox: some View {
        TextField("Enter a number", text: $thresholdVelocity)
            .focused($isActive)
            .onSubmit {
                isActive = false
                testVelocity = thresholdVelocity
                print("Inputted Velocity: \(testVelocity)")
                if let _ = Double(thresholdVelocity){
                    validVelocity = true
                    print("Valid Velocity")
                    
                }
                else{
                    validVelocity = false
                    print("Invalid Velocity")
                }
            }
    }
    
    var validVelocityText: some View {
        if validVelocity{
            return Text("Inputted Velocity: \(thresholdVelocity)")
        }
        else{
            return Text("Input is invalid!")
                .foregroundStyle(.red)
        }
    }
    
    var thresholdControl: some View {
        VStack{
            if thresholdSetting{
                Text("Stay Over Threshold")
                    .font(.title)
                    .bold()
            }
            else{
                Text("Stay Under Threshold")
                    .font(.title)
                    .bold()
            }
            Toggle("",
                   systemImage: "lightbulb.led",
                   isOn: $thresholdSetting
            )
            .foregroundStyle(.primary)
            .onChange(of: thresholdSetting) { _, newValue in
                bleManager.sendTextValue(newValue ? "1":"0" )
            }
        }
    }
    
    // Old Elements //

    var ledControl: some View {
        Toggle("Led",
               systemImage: "lightbulb.led",
               isOn: $isEsp32LedEnabled
        )
        .foregroundStyle(.primary)
        .onChange(of: isEsp32LedEnabled) { _, newValue in
            bleManager.sendTextValue(newValue ? "Hello World":"Goodbye World" )
        }
    }

    var sevenSegmentDisplayControl: some View {
        Stepper(value: $sevenSegmentValue,
                in: 0...9,
                step: 1) {
            HStack(spacing: 4) {
                Text(sevenSegmentValue.description)
            }
        }
        .onChange(of: sevenSegmentValue) { _, newValue in
            bleManager.sendTextValue(newValue.description)
        }
    }

    var trafficLight: some View {
        VStack(alignment: .leading) {
            Toggle("Auto",
                   isOn: $isAutoSwitched
            )
            Divider()
            Picker("Traffic Light", selection: $trafficLightColor) {
                Text("Red").tag("RED")
                Text("Yellow").tag("YELLOW")
                Text("Green").tag("GREEN")
            }
            .frame(height: 50)
            .pickerStyle(.segmented)
            .tint(.blue)
            .onChange(of: trafficLightColor) { _, newValue in
                bleManager.sendTextValue(newValue.uppercased())
            }
            .onChange(of: isAutoSwitched) { _, newValue in
                switch newValue {
                case true:
                    cancellable = timer.autoconnect().sink { _ in
                        switch trafficLightColor {
                        case "RED":
                            trafficLightColor = "YELLOW"
                            bleManager.sendTextValue("YELLOW")
                        case "YELLOW":
                            trafficLightColor = "GREEN"
                            bleManager.sendTextValue("GREEN")
                        case "GREEN":
                            trafficLightColor = "RED"
                            bleManager.sendTextValue("RED")
                        default:
                            trafficLightColor = "RED"
                        }
                    }
                case false:
                    cancellable?.cancel()
                }
            }
        }
    }

    var receivedBoolen: some View {
        VStack(alignment: .leading) {
            Text(Image(systemName: bleManager.bootButonState ? "button.programmable" : "circle"))
            +
            Text("  \(bleManager.bootButonState ? "Pressed" : "Released")")
        }
    }

    func device(_ peripheral: Peripheral) -> some View {
        VStack(alignment: .leading) {
            HStack {
                Text(peripheral.name)
                Spacer()
                Button(action: {
                    bleManager.connectPeripheral(peripheral: peripheral)
                }) {
                    Text("Connect")
                }
                .buttonStyle(.borderedProminent)
            }

            Divider()

            VStack(alignment: .leading) {
                Group {
                    Text("""
                          Device UUID:
                          \(peripheral.id.uuidString)
                          """)
                    .padding([.bottom], 10)

                    if let adsServiceUUIDs = peripheral.advertisementServiceUUIDs {
                        Text("Advertisement Service UUIDs:")
                        ForEach(adsServiceUUIDs, id: \.self) { uuid in
                            Text(uuid)
                        }
                    }

                    HStack {
                        Image(systemName: "chart.bar.fill")
                        Text("\(peripheral.rssi) dBm")
                    }
                    .padding([.top], 10)
                }
                .font(.footnote)
            }
        }
    }
}

struct VelocityGraphView: View {
    @State var timePoint : Double?
    var thresholdVelocityGraph : Double
    var unidentifiableData : [(Double, Double)]
    var identifiableData: [VelocityPoint] {
        unidentifiableData.map { VelocityPoint(time: $0.0, velocity: $0.1) }
    }
    
    
//    var selectedVelocity: Double? {
//        guard let timePoint else {
//            print("Bad Time: Error 1")
//            return 0
//        }
//        
//        var low = 0
//        var high = identifiableData.count - 1
//        var rounded_val = round(1000*timePoint)/1000
//
//        while low <= high {
//            let mid = (low + high) / 2
//            let midTime = identifiableData[mid].time
//            
//            if mid + 1 < identifiableData.count {
//                let nextTime = identifiableData[mid + 1].time
//                if midTime <= rounded_val && rounded_val <= nextTime{
//                    return identifiableData[mid].velocity
//                }
//            }
//            
//            if timePoint < midTime {
//                high = mid - 1
//            } else {
//                low = mid + 1
//            }
//        }
//
//        return identifiableData.last?.velocity ?? 0
//    }
    
    var body: some View {
        VStack{
            Text("Velocity")
                .font(.title)
                .bold()
            Chart(identifiableData){
                RuleMark(y: .value("Threshold Velocity", thresholdVelocityGraph)) // acquired from https://www.youtube.com/watch?v=4utsyqhnS4g&ab_channel=SeanAllen
                    .foregroundStyle(Color.red)
                    .lineStyle(StrokeStyle(lineWidth: 1, dash: [5]))
                LineMark(x: .value("Time", $0.time), y: .value("Velocity", $0.velocity))
                    .interpolationMethod(.catmullRom)
                // Although the graph does not seem that smooth, it helps with getting velocity values
                
                if let timePoint{
                    RuleMark(x: .value("Time", timePoint))
                        .foregroundStyle(.secondary.opacity(0.3))
                        .lineStyle(StrokeStyle(lineWidth: 1, dash: [5]))
                }
            }
            .frame(width: UIScreen.main.bounds.width, height: 400)
            .chartScrollableAxes(.horizontal) // from https://www.youtube.com/watch?v=9nQh5QofS8c&ab_channel=StewartLynch
            
//            .chartXSelection(value: $timePoint)
        }
    }
    
}

struct AngleGraphView: View {
    @State var timePoint : Double?
    var unidentifiableData : [(Double, Double)]
    var identifiableData: [AnglePoint] {
        unidentifiableData.map { AnglePoint(time: $0.0, angle: $0.1) }
    }
    
//    var selectedAngle: Double? {
//        guard let timePoint else {
//            print("Bad Time: Error 1")
//            return 0
//        }
//        
//        var low = 0
//        var high = identifiableData.count - 1
//
//        while low <= high {
//            let mid = (low + high) / 2
//            let midTime = identifiableData[mid].time
//            
//            if mid + 1 < identifiableData.count {
//                let nextTime = identifiableData[mid + 1].time
//                if midTime <= timePoint && timePoint <= nextTime{
//                    return identifiableData[mid].angle
//                    
//                }
//            }
//            
//            if timePoint < midTime {
//                high = mid - 1
//            } else {
//                low = mid + 1
//            }
//        }
//
//        return identifiableData.last?.angle ?? 0
//    }
    
    var body: some View {
        VStack{
            Text("Angle")
                .font(.title)
                .bold()
            Chart(identifiableData){
                RuleMark(y: .value("Threshold Velocity", 25)) // acquired from https://www.youtube.com/watch?v=4utsyqhnS4g&ab_channel=SeanAllen
                    .foregroundStyle(Color.red)
                    .lineStyle(StrokeStyle(lineWidth: 1, dash: [5]))
                
                RuleMark(y: .value("Threshold Velocity", -25)) // acquired from https://www.youtube.com/watch?v=4utsyqhnS4g&ab_channel=SeanAllen
                    .foregroundStyle(Color.red)
                    .lineStyle(StrokeStyle(lineWidth: 1, dash: [5]))
                
                LineMark(x: .value("Time", $0.time), y: .value("Angle", $0.angle))
                    .interpolationMethod(.catmullRom)
                
                if let timePoint{
                    RuleMark(x: .value("Time", timePoint))
                        .foregroundStyle(.secondary.opacity(0.3))
                        .lineStyle(StrokeStyle(lineWidth: 1, dash: [5]))
                    
                }
            }
        }
        .frame(width: UIScreen.main.bounds.width, height: 400)
        .chartScrollableAxes(.horizontal) // from https://www.youtube.com/watch?v=9nQh5QofS8c&ab_channel=StewartLynch
//        .chartXSelection(value: $timePoint)
    }
    
}


// Data struct for points for velocity/angle graphs
struct VelocityPoint: Identifiable{
    var time: Double
    var velocity: Double
    var id = UUID()
}

struct AnglePoint: Identifiable{
    var time: Double
    var angle: Double
    var id = UUID()
}


