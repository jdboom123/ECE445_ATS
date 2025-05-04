//
//  info.swift
//  ESP32 BLE
//
//  Created by John Del Armedilla on 4/27/25.
//

import SwiftUI

struct InfoView: View {
    var body: some View {
        VStack(alignment: .leading){
            Text("How to Use Athletic Tracking Sensor")
                .font(.title)
                .bold()
                .padding()
                .frame(alignment: .top)
                .foregroundColor(/*@START_MENU_TOKEN@*/.blue/*@END_MENU_TOKEN@*/)

            
            Text("1) Set a exercise and velocity threshold on the app.")
                .padding()
                .foregroundColor(/*@START_MENU_TOKEN@*/.blue/*@END_MENU_TOKEN@*/)
            
            Text("2) Press the \"Send Data\" button.")
                .padding()
                .foregroundColor(.blue)
            
            Text("3) On the Tracking Sensor, press the button and wait for a second for recording to start.")
                .padding()
                .foregroundColor(/*@START_MENU_TOKEN@*/.blue/*@END_MENU_TOKEN@*/)
            
            Text("4) Perform your exercise. Once satisfied, press the button on the Tracking Sesnsor again to stop recording")
                .padding()
                .foregroundColor(/*@START_MENU_TOKEN@*/.blue/*@END_MENU_TOKEN@*/)
            
            Text("5) Press the \"Present Velocity Graph\" to present your velocity over time or the \"Present Angle Graph\" to present your orientation over time.")
                .padding()
                .foregroundColor(/*@START_MENU_TOKEN@*/.blue/*@END_MENU_TOKEN@*/)
            
            Spacer()
        }
    }

}

#Preview {
    InfoView()
}
